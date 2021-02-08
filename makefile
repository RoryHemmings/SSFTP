#
#	Makefile
#
#	Author: Rory Hemmings
#

CC=g++
CFLAGS=-I. -fopenmp
DEPS = sftp.h socket.h logger.h

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sftp: sftp.o socket.o logger.o clientMain.o serverMain.o
	$(CC) -o client sftp.o clientMain.o socket.o logger.o
	$(CC) -o server sftp.o serverMain.o socket.o logger.o


