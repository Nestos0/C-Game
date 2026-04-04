CC = gcc

CFLAGS = -Wall -g

Project = DND-Like

TARGET = game

SRCS = display.c game.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(SRCS) -o $(TARGET) $(CFLAGS) -O2

clean:
	rm -f $(TARGET)
