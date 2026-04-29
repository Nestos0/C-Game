CC = gcc

CFLAGS = -Wall -g

TARGET = Zi-Game

SRCS = $(wildcard src/*.c) main.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) -std=gnu23 -o build/$(TARGET) $(SRCS) -I include/ -O2

run:
	$(CC) -o build/$(TARGET) $(SRCS)
	./build/$(TARGET)

clean:
	rm -rf ./build/
