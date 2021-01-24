#
# Makefile
# Version 0.3
#
# Author: Rory Hemmings
#

CC=g++
CFLAGS=-I .
DEPS = utils.h

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

proof-of-concept: utils.o client.o server.o
	$(CC) -o client client.o utils.o
	$(CC) -o server server.o utils.o

