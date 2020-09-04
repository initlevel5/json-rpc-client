CC = gcc

CFLAGS = -W -Wall -pedantic -O3 -std=c99

OBJS = \
	json_rpc.o \
	main.o

TARGET = client

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) -v $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJS) $(TARGET)