#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "utils.h"
#include "timer.h"
#include "packet.h"
#include "arp.h"
#include "tcp.h"
#include "tcpsockm.h"

#pragma pack(1)
struct File
{
    uint32_t size;
    char name[13];
    char unused[15];
};

const int ListenPort = 2000;
const uint16_t RCV_BUF_SIZE = 8192;
const int BUFFER_SIZE = 8192;
int ctrlBreakDetected = 0;
TcpSocket *socket = 0;
struct File file;

char escape_pressed()
{
    unsigned char keypress = 0;

    __asm
    {
        in al, 60h
        mov keypress, al
    }

    return keypress == 1;
}

void (__interrupt __far* oldCtrlBreakHandler)();
void (__interrupt __far* oldCtrlCHandler)();

void __interrupt __far ctrlBreakHandler()
{
    ctrlBreakDetected = 1;
}

void shutdown()
{
    if (socket)
    {
        socket->close();
        TcpSocketMgr::freeSocket(socket);
    }

    setvect(0x1b, oldCtrlBreakHandler);
    setvect(0x23, oldCtrlCHandler);
    Utils::endStack();

    exit(1);
}

// Blocking read. Returns 1 on success, 0 on failure.
int read_blocking(TcpSocket* socket, void* buffer, uint32_t size)
{
    uint32_t numread = 0;
    while (numread != size)
    {
        if (ctrlBreakDetected || escape_pressed() || socket->isRemoteClosed())
            return 0;

        PACKET_PROCESS_SINGLE;
        Arp::driveArp();
        Tcp::drivePackets();

        while (1)
        {
            int16_t recvRc = socket->recv((uint8_t*)(buffer)+numread, (uint16_t)(size - numread));
            if (recvRc > 0)
            {
                numread += recvRc;
                if (numread == size)
                    break;
            }
            else
            {
                break;
            }
        }
    }

    return 1;
}

int main()
{
    if (Utils::parseEnv() != 0)
    {
        printf("error: failed to parse mtcp config\n");
        exit(1);
    }

    if (Utils::initStack(2, TCP_SOCKET_RING_SIZE))
    {
        printf("error: failed to init\n");
        exit(1);
    }

    oldCtrlBreakHandler = getvect(0x1b);
    oldCtrlCHandler = getvect(0x23);
    setvect(0x1b, ctrlBreakHandler);
    setvect(0x23, ctrlBreakHandler);

    TcpSocket* listeningSocket = TcpSocketMgr::getSocket();
    listeningSocket->listen(ListenPort, RCV_BUF_SIZE);

    uint8_t* fileWriteBuffer = (uint8_t*)malloc(BUFFER_SIZE);

    printf("waiting for connection..\n");
    int aborted = 0;
    while (1)
    {
        if (ctrlBreakDetected)
        {
            aborted = 1;
            break;
        }

        PACKET_PROCESS_SINGLE;
        Arp::driveArp();
        Tcp::drivePackets();

        socket = TcpSocketMgr::accept();
        if (socket != NULL)
        {
            // Client connected.
            listeningSocket->close();
            TcpSocketMgr::freeSocket(listeningSocket);
            break;
        }

        if (escape_pressed())
        {
            aborted = 1;
            break;
        }
    }

    if (aborted != 0)
        shutdown();

    printf("client connected..\n");

    uint16_t bytesRead = 0;
    uint16_t bytesToRead = BUFFER_SIZE;
    uint32_t totalBytesReceived = 0;
    char readingFile = 0;
    FILE* fp = 0;

    while (1)
    {
        if (ctrlBreakDetected || escape_pressed())
            break;

        PACKET_PROCESS_SINGLE;
        Arp::driveArp();
        Tcp::drivePackets();

        if (readingFile)
        {
            while (1)
            {
                if (ctrlBreakDetected || escape_pressed())
                    break;

                uint32_t fileBytesRemaining = file.size - totalBytesReceived;
                uint16_t actualBytesToRead = fileBytesRemaining < bytesToRead ? fileBytesRemaining : bytesToRead;
                int16_t recvRc = socket->recv(fileWriteBuffer + bytesRead, actualBytesToRead);

                if (recvRc > 0)
                {
                    // Write new bytes to the file.
                    fwrite(fileWriteBuffer + bytesRead, 1, recvRc, fp);

                    totalBytesReceived += recvRc;
                    bytesRead += recvRc;
                    bytesToRead -= recvRc;

                    if (totalBytesReceived == file.size)
                    {
                        fflush(fp);
                        fclose(fp);
                        readingFile = 0;
                        break;
                    }

                    if (bytesToRead == 0)
                    {
                        // Keep reading from the start of the buffer again.
                        bytesToRead = BUFFER_SIZE;
                        bytesRead = 0;
                    }
                }
                else
                {
                    // No data or error.
                    break;
                }
            }

            int progress = (totalBytesReceived*10) / file.size;
            printf("\r[");
            for (int j = 0; j < 10; j++)
            {
                if (j < progress)
                {
                    printf("%c", 178);
                }
                else
                {
                    printf("%c", '-');
                }
            }
            printf("]");

            if (!readingFile)
                printf("\n");
        }
        else
        {
            // Wait for a file to be sent.
            if (!read_blocking(socket, &file, sizeof(file)))
                break;

            printf("receiving %s, %lu bytes\n", file.name, (unsigned long)file.size);
            bytesToRead = BUFFER_SIZE;
            totalBytesReceived = 0;
            bytesRead = 0;
            readingFile = 1;
            fp = fopen(file.name, "wb");
        }

        if (socket->isRemoteClosed())
        {
            printf("remote closed connection\n");
            break;
        }
    }

    shutdown();
    return 0;
}
