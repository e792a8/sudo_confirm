VERSION = 0.1.1

CC = gcc
CFLAGS = -DPACKAGE_VERSION=\"$(VERSION)\" -shared -fPIC

.PHONY: all clean
all: sudo_confirm.so

clean:
	-rm sudo_confirm.so

sudo_confirm.so: sudo_confirm.c
	$(CC) $(CFLAGS) $< -o $@
