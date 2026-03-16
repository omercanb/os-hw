

CC	:= gcc
CFLAGS := -g -Wall

TARGETS :=  libtus.a get app
# Make sure that 'all' is the first target
all: $(TARGETS)

TUS_SRC :=  tus.c
TUS_OBJS := $(TUS_SRC:.c=.o)

libtus.a: $(TUS_OBJS)
	ar rcs $@ $(TUS_OBJS)

TUS_LIB :=  -L . -l tus


get: get.c
	gcc $(CFLAGS) -o $@ $<

tus.o: tus.c tus.h
	gcc -c $(CFLAGS)  -o $@ tus.c


app.o: app.c  tus.h
	gcc -c $(CFLAGS)  -o $@ app.c

app: app.o libtus.a
	gcc $(CFLAGS) -o $@ app.o  $(TUS_LIB)

clean:
	rm -rf core  *.o *.out *~ $(TARGETS)
	
	
