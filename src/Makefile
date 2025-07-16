BIN:=esp-ubus-module
LDFLAGS:= \
	-lblobmsg_json \
	-lubox \
	-lubus \
	-luci \
	-lserialport
CPPFLAGS:= -g -I$(CURDIR) -Wall -Wextra -Werror
SRC:= $(wildcard *.c)
OBJS:= $(SRC:.c=.o)

.PHONY:= all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BIN)