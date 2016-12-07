CFLAGS=-Wall -std=c99 -pedantic -D_POSIX_C_SOURCE=200112L

all: shell myls/myls

shell: shell.c
	gcc -o shell $(CFLAGS) shell.c

myls/myls: myls/myls.c
	gcc -o myls/myls $(CFLAGS) myls/myls.c

.PHONY: clean

clean:
	rm -f shell
	rm -f myls/myls