CC = gcc
CFLAGS = -I. -Wall -O2
LDLIBS = -pthread

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCS))
TARGET = umicat

.PHONY: all clean debug

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(LDLIBS) -o $@ $^

$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) -c $^

debug: $(SRCS)
	$(CC) -I. -Wall -pthread -O0 -g -o umicat $^

clean:
	rm -f $(OBJS) $(TARGET) umicat.log