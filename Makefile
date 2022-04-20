CC = gcc
CFLAGS = -I${DIR_INC} -Wall -O2
CFLAGS_DEBUG = -I${DIR_INC} -Wall -O0 -g
LDLIBS = -pthread

PREFIX = /usr/local

DIR_SRC = src
DIR_INC = src
DIR_OBJ = objs

SRCS = $(wildcard ${DIR_SRC}/*.c)
INCS = $(wildcard $(DIR_INC)/*.h)
OBJS = $(patsubst %.c, ${DIR_OBJ}/%.o, $(notdir ${SRCS}))
TARGET = umicat

.PHONY: all clean install uninstall test debug

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(LDLIBS) -o $@ $^

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c | $(DIR_OBJ)
	$(CC) $(CFLAGS) -o $@ -c $<

$(DIR_OBJ):
	mkdir -p $@

install:
	install -Dm755 "umicat" "$(PREFIX)/bin/umicat"
	test -d '$(PREFIX)/etc/umicat' || mkdir -p "$(PREFIX)/etc/umicat"
	cat "conf/umicat.conf" > "$(PREFIX)/etc/umicat/umicat.conf"
	test -d '/var/log/umicat' || mkdir -p "/var/log/umicat"
	cat "systemd/umicat.service" > "/etc/systemd/system/umicat.service"
	systemctl daemon-reload
	systemctl enable "/etc/systemd/system/umicat.service"

uninstall:
	systemctl stop "umicat.service"
	systemctl disable "umicat.service"
	rm -f "/etc/systemd/system/umicat.service"
	systemctl daemon-reload
	rm -f "$(PREFIX)/bin/umicat"
	rm -rf "$(PREFIX)/etc/umicat"
	rm -rf "/var/log/umicat"

test: debug
	./umicat -c conf/umicat.conf -l umicat.log

debug: $(SRCS)
	$(CC) ${CFLAGS_DEBUG} $(LDLIBS) -o $(TARGET) $^

clean:
	rm -rf $(DIR_OBJ) $(TARGET) umicat.log