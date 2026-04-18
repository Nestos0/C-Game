CC = gcc

CFLAGS = -Wall -g

Project = DND-Like

TARGET = game

SRCS = display.c game.c utils.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(SRCS) -o $(TARGET) $(CFLAGS) -I include/ -O2

clean:
	rm -f $(TARGET)
