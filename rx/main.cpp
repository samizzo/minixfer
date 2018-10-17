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
const int WRITE_BUF_SIZE = 8192;
const int READ_BUF_SIZE  = 8192;
int CtrlBreakDetected = 0;
TcpSocket *mySocket = 0;

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

void ( __interrupt __far *oldCtrlBreakHandler)( );

void __interrupt __far ctrlBreakHandler( ) {
  CtrlBreakDetected = 1;
}

void __interrupt __far ctrlCHandler( ) {
  // Do Nothing
}

void shutdown()
{
    if (mySocket)
    {
        mySocket->close();
        TcpSocketMgr::freeSocket(mySocket);
    }

    setvect( 0x1b, oldCtrlBreakHandler);
    Utils::endStack( );

    exit(1);
}

// Blocking read. Returns 1 on success, 0 on failure.
int read_blocking(TcpSocket* socket, void* buffer, uint32_t size)
{
    uint32_t numread = 0;
    while (numread != size)
    {
        if (CtrlBreakDetected || escape_pressed())
            return 0;

        PACKET_PROCESS_SINGLE;
        Arp::driveArp();
        Tcp::drivePackets();

        while (1)
        {
            int16_t recvRc = mySocket->recv((uint8_t*)(buffer)+numread, (uint16_t)(size - numread));
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
        printf("failed to parse mtcp config\n");
        exit(1);
    }

    if (Utils::initStack(2, TCP_SOCKET_RING_SIZE))
    {
        printf("failed to init\n");
        exit(1);
    }

    // Save off the oldCtrlBreakHander and put our own in.  Shutdown( ) will
    // restore the original handler for us.
    oldCtrlBreakHandler = getvect( 0x1b );
    setvect( 0x1b, ctrlBreakHandler);

    // Get the Ctrl-C interrupt too, but do nothing.  We actually want Ctrl-C
    // to be a legal character to send when in interactive mode.
    setvect( 0x23, ctrlCHandler);

    TcpSocket *listeningSocket = TcpSocketMgr::getSocket();
    listeningSocket->listen(ListenPort, RCV_BUF_SIZE);

    uint8_t *fileWriteBuffer = (uint8_t *)malloc( WRITE_BUF_SIZE );

    printf("Waiting for connection..\n");
    int rc = 0;
    while (1)
    {
        if (CtrlBreakDetected )
        {
            rc = -1;
            break;
        }

        PACKET_PROCESS_SINGLE;
        Arp::driveArp();
        Tcp::drivePackets( );

        mySocket = TcpSocketMgr::accept();
        if (mySocket != NULL)
        {
            listeningSocket->close();
            TcpSocketMgr::freeSocket(listeningSocket);
            rc = 0;
            break;
        }

        if (escape_pressed())
        {
            rc = -1;
            break;
        }
    }

    if (rc != 0)
        shutdown();

    printf("Client connected\n");

    int maxPacketSize = MyMTU - (sizeof(IpHeader) + sizeof(TcpHeader));
    uint16_t bytesRead = 0;
    uint16_t bytesToRead = WRITE_BUF_SIZE; // Bytes to read from socket
    uint32_t totalBytesReceived = 0;
    char readingFile = 0;
    FILE* fp = 0;

    while (1)
    {
        if (CtrlBreakDetected || escape_pressed())
            break;

        PACKET_PROCESS_SINGLE;
        Arp::driveArp();
        Tcp::drivePackets();

        if (readingFile)
        {
            while (1)
            {
                if (CtrlBreakDetected || escape_pressed())
                    break;

                uint32_t fileBytesRemaining = file.size - totalBytesReceived;
                uint16_t actualBytesToRead = fileBytesRemaining < bytesToRead ? fileBytesRemaining : bytesToRead;
                int16_t recvRc = mySocket->recv(fileWriteBuffer+bytesRead, actualBytesToRead);

                if (recvRc > 0)
                {
                    // Write new bytes to the file.
                    fwrite(fileWriteBuffer+bytesRead, 1, recvRc, fp);

                    totalBytesReceived += recvRc;
                    bytesRead += recvRc;
                    bytesToRead -= recvRc;

                    if (totalBytesReceived == file.size)
                    {
                        printf("Finished reading file\n");
                        fflush(fp);
                        fclose(fp);
                        readingFile = 0;
                        break;
                    }

                    if (bytesToRead == 0)
                    {
                        // Keep reading from the start of the buffer again.
                        bytesToRead = WRITE_BUF_SIZE;
                        bytesRead = 0;
                    }
                }
                else
                {
                    // No data or an error - break this local receive loop
                    break;
                }
            }
        }
        else
        {
            if (!read_blocking(mySocket, &file, sizeof(file)))
                break;
            printf("Reading file %s, %lu bytes\n", file.name, (unsigned long)file.size);
            bytesToRead = WRITE_BUF_SIZE;
            bytesRead = 0;
            readingFile = 1;
            fp = fopen(file.name, "wb");
        }

        if (mySocket->isRemoteClosed())
        {
            printf("Remote closed connection\n");
            break;
        }
    }

    shutdown();
    return 0;
}
