#
#	Makefile
#
#	Author: Rory Hemmings
#

CC=g++
CFLAGS=-I. -fopenmp
DEPS = socket.h logger.h

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sftp: socket.o logger.o clientMain.o serverMain.o
	$(CC) -o client clientMain.o socket.o logger.o
	$(CC) -o server serverMain.o socket.o logger.o

