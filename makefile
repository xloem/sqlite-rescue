CC=gcc
CFLAGS=-O3 -flto -fwhole-program -Wall -Wextra
#CFLAGS=-O0 -ggdb -Wall -Wextra -Wpedantic -fsanitize=address
LDFLAGS=-flto -fwhole-program

scan-db-guix: scan-db-guix.c sqlite.c sqlite_search.c

# "plugin needed to handle lto"
#sqlite_search.a: sqlite.o sqlite_search.o
#	ar -cru $@ $^
