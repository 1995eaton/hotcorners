CC ?= gcc

CFLAGS += -std=gnu99 -Wall -Wextra -O2
LIBS = -lX11 -lpthread
LDFLAGS += -s

PREFIX ?= /usr/local
BINPREFIX = $(PREFIX)/bin

SRC = config-parser.c hotcorners.c
OBJ = $(SRC:.c=.o)
EXE = hotcorners

all: $(EXE)

$(OBJ) $(SRC): Makefile

.c.o:
	$(CC) $(CFLAGS) $(OPTFLAGS) -c -o $@ $<

$(EXE): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS) $(LIBS)

install:
	install -d -m 0755 "$(DESTDIR)$(BINPREFIX)"
	install -m 0755 $(EXE) "$(DESTDIR)$(BINPREFIX)"

uninstall:
	rm -f "$(DESTDIR)$(BINPREFIX)"/$(EXE)

clean:
	rm -f $(OBJ) $(EXE)
