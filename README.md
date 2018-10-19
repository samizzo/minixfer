minixfer
--------

minixfer is a minimal transfer tool for sending files between two machines
on a network. Currently the transmission program 'tx' is designed for use
under Windows, and the receiving program 'rx' is designed for use under
MS-DOS.

'rx' was built using Open Watcom 1.9 and the mTCP library. The makefile
has stubs for building for Windows but this is not implemented.

'tx' was built using Visual Studio 2017. There is a makefile for building
under MS-DOS but this is not implemented.

Usage
-----

On the receiving machine, run rx.exe. On the sending machine, run:

tx.exe <ip address> <filename1> <filename2> ... <filenameN>
