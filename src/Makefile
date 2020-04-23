CC = gcc
CFLAGS = -ansi -Wall -Wextra -Werror -pedantic-errors -I/usr/local/lib/gurobi563/include -O3
LDFLAGS = -L/usr/local/lib/gurobi563/lib -lgurobi56

OBJS = backtrack.o board.o checked_alloc.o history.o list.o lp.o main.o mainaux.o parser.o
EXEC = sudoku-console

backtrack.o: backtrack.c backtrack.h board.h bool.h
	$(CC) $(CFLAGS) -c $*.c

board.o: board.c board.h bool.h checked_alloc.h
	$(CC) $(CFLAGS) -c $*.c

checked_alloc.o: checked_alloc.c checked_alloc.h
	$(CC) $(CFLAGS) -c $*.c

history.o: history.c history.h board.h bool.h list.h checked_alloc.h
	$(CC) $(CFLAGS) -c $*.c

list.o: list.c list.h bool.h checked_alloc.h
	$(CC) $(CFLAGS) -c $*.c

lp.o: lp.c lp.h board.h bool.h checked_alloc.h
	$(CC) $(CFLAGS) -c $*.c

main.o: main.c board.h bool.h game.h history.h lp.h mainaux.h parser.h list.h
	$(CC) $(CFLAGS) -c $*.c

mainaux.o: mainaux.c mainaux.h bool.h game.h parser.h board.h history.h list.h lp.h backtrack.h checked_alloc.h
	$(CC) $(CFLAGS) -c $*.c

parser.o: parser.c parser.h game.h bool.h checked_alloc.h
	$(CC) $(CFLAGS) -c $*.c

$(EXEC): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

all: $(EXEC)

clean:
	rm -f $(OBJS) $(EXEC)
