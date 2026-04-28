CC = gcc

SRC = main.c

TARGET = main

$(TARGET):
	$(CC) -o $(TARGET) $(SRC)
