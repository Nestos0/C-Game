CC      = gcc
CFLAGS  = -std=gnu23 -Wall -g -O2 -I include/
TARGET  = Zi-Game
BUILD_DIR = build

SRCS    = $(shell find src/ -name '*.c') main.c

.PHONY: all run clean debug

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(BUILD_DIR)/$(TARGET)

run: all
	./$(BUILD_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
