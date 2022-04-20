CC = gcc
CFLAGS = -I${DIR_INC} -Wall -O2
CFLAGS_DEBUG = -I${DIR_INC} -Wall -O0 -g
LDLIBS = -pthread

DIR_SRC = src
DIR_INC = src
DIR_OBJ = obj

SRCS = $(wildcard ${DIR_SRC}/*.c)
INCS = $(wildcard $(DIR_INC)/*.h)
OBJS = $(patsubst %.c, ${DIR_OBJ}/%.o, $(notdir ${SRCS}))
TARGET = umicat

.PHONY: all clean install uninstall debug

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(LDLIBS) -o $@ $^

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c | $(DIR_OBJ)
	$(CC) $(CFLAGS) -o $@ -c $<

$(DIR_OBJ):
	mkdir -p $@

debug: $(SRCS)
	$(CC) ${CFLAGS_DEBUG} $(LDLIBS) -o $(TARGET) $^

clean:
	rm -rf $(DIR_OBJ) $(TARGET) umicat.log