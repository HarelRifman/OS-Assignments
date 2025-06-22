CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread
TARGET = ex3.out
OBJS = main.o bounded_buffer.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c bounded_buffer.h
	$(CC) $(CFLAGS) -c main.c

bounded_buffer.o: bounded_buffer.c bounded_buffer.h
	$(CC) $(CFLAGS) -c bounded_buffer.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean