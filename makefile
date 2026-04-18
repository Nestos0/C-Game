CC = gcc

CFLAGS = -Wall -g

Project = DND-Like

TARGET = game

SRCS = $(wildcard src/*.c) main.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(SRCS) -o $(TARGET) $(CFLAGS) -I include/ -O2

clean:
	rm -f $(TARGET)
