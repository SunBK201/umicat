CC = gcc
CFLAGS = -O2 -Wall -I .
LDFLAGS = -I . -pthread

SRCS = $(wildcard *.c)

OBJS = $(patsubst %c, %o, $(SRCS))

TARGET = umicat

.PHONY:all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o:%.c
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(OBJS) $(TARGET) umicat.log