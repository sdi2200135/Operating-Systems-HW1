CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
TARGET = message_system
SRC_DIR = src
INCLUDE_DIR = include
OBJS = $(SRC_DIR)/main.o $(SRC_DIR)/shared_memory.o $(SRC_DIR)/synchronization.o $(SRC_DIR)/dialog_control.o $(SRC_DIR)/msg_queue.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(SRC_DIR)/main.o: $(SRC_DIR)/main.c $(INCLUDE_DIR)/common.h $(INCLUDE_DIR)/shared_memory.h $(INCLUDE_DIR)/dialog_control.h $(INCLUDE_DIR)/msg_queue.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_DIR)/shared_memory.o: $(SRC_DIR)/shared_memory.c $(INCLUDE_DIR)/shared_memory.h $(INCLUDE_DIR)/common.h $(INCLUDE_DIR)/synchronization.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_DIR)/synchronization.o: $(SRC_DIR)/synchronization.c $(INCLUDE_DIR)/synchronization.h $(INCLUDE_DIR)/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_DIR)/dialog_control.o: $(SRC_DIR)/dialog_control.c $(INCLUDE_DIR)/dialog_control.h $(INCLUDE_DIR)/common.h $(INCLUDE_DIR)/synchronization.h $(INCLUDE_DIR)/shared_memory.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_DIR)/msg_queue.o: $(SRC_DIR)/msg_queue.c $(INCLUDE_DIR)/msg_queue.h $(INCLUDE_DIR)/common.h $(INCLUDE_DIR)/synchronization.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run	