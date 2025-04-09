PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin
MANDIR := $(PREFIX)/share/man/man1

CC := gcc
CFLAGS := -Wall -Wextra

BINARY := casectl
SOURCE := casectl.c

.PHONY: all hash clean install uninstall

all: $(BINARY)

hash: $(BINARY)
	sha256sum $(BINARY) > $(BINARY).sha256

$(BINARY): $(SOURCE)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BINARY) $(BINARY).o

install: $(BINARY) $(BINARY).1
	install -Dm755 $(BINARY) $(BINDIR)/$(BINARY)
	install -Dm644 $(BINARY).1 $(MANDIR)/$(BINARY).1
	@echo "$(BINARY): installed to $(BINDIR), manpage to $(MANDIR)"

uninstall:
	rm -f $(BINDIR)/$(BINARY)
	rm -f $(MANDIR)/$(BINARY).1
	@echo "$(BINARY): uninstalled"

