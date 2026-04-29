CC = gcc

CFLAGS = -Wall -g

TARGET = Zi-Game

SRCS = $(shell find src/ -name '*.c') main.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) -std=gnu23 -o build/$(TARGET) $(SRCS) -I include/ -O2

run:
	$(CC) -std=gnu23 -o build/$(TARGET) $(SRCS) -I include/ -O2
	./build/$(TARGET)

clean:
	rm -rf ./build/
