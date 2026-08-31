CC       ?= gcc
BIN      := klok
PREFIX   ?= /usr/local
SRC      := $(wildcard src/*.c)
OBJ      := $(SRC:.c=.o)
CFLAGS   ?= -Wall -Wextra -std=gnu11 -O2
LDLIBS   := -lncursesw

# Build with weather support (requires libcurl + its headers):
#   make WITH_WEATHER=1
WITH_WEATHER ?= 0
ifeq ($(WITH_WEATHER),1)
CFLAGS  += -DWITH_WEATHER $(shell pkg-config --cflags libcurl 2>/dev/null)
LDLIBS  += $(shell pkg-config --libs libcurl 2>/dev/null || echo -lcurl)
endif

.PHONY: all clean install uninstall

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

src/%.o: src/%.c src/*.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(BIN)

install: $(BIN)
	install -Dm755 $(BIN) $(DESTDIR)$(PREFIX)/bin/$(BIN)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)
