#
#	Makefile
#	Author: Rory Hemmings
#

CC=g++
CFLAGS=-I. -fopenmp
DEPS = sftp.h socket.h connection.h logger.h utils.h

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sftp: sftp.o socket.o connection.o logger.o utils.o clientMain.o serverMain.o
	$(CC) -o client sftp.o clientMain.o socket.o logger.o utils.o
	$(CC) -o server sftp.o serverMain.o socket.o connection.o logger.o utils.o -lpthread -lcrypt


