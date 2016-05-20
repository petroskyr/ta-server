# $gcc -Wall -o main -ggdb dir_check.c main.c

CC= gcc
CFLAGS= -Wall -ggdb
LFLAGS= -ggdb -lpthread

OBJS= main.o set_up.o netwrk.o
EX= main

.PHONY : all
all : $(EX)

$(EX) : $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS)

%.o : %.c %.h
	$(CC) -c $(CFLAGS) $< -o $@




.PHONY : clean
clean :
	rm -f $(OBJS) $(EX)



