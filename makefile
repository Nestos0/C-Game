CC = gcc

SRC = main.c

TARGET = main

$(TARGET):
	$(CC) -o build/$(TARGET) $(SRC)
