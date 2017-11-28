CC = gcc
DEBUG = -g

dphil: dphil.o
	$(CC) $(DEBUG) -o dphil dphil.o

dphil.o: dphil.c dphil.h
	$(CC) $(DEBUG) -c dphil.c