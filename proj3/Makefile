# librsm.a:  rsm.c
# 	gcc -Wall -c rsm.c
# 	ar -cvq librsm.a rsm.o
# 	ranlib librsm.a

CFLAGS = -Wall -ggdb -fsanitize=address

all: librsm.a app myapp exp1 exp2 exp3

librsm.a: rsm.c
	gcc $(CFLAGS) -c rsm.c
	ar -cvq librsm.a rsm.o
	ranlib librsm.a

app: app.c
	gcc $(CFLAGS) -o app app.c -L. -lrsm -lpthread

myapp: myapp.c
	gcc $(CFLAGS) -o myapp myapp.c -L. -lrsm -lpthread

exp1: exp1.c
	gcc $(CFLAGS) -o exp1 exp1.c -L. -lrsm -lpthread

exp2: exp2.c
	gcc $(CFLAGS) -o exp2 exp2.c -L. -lrsm -lpthread

exp3: exp3.c
	gcc $(CFLAGS) -o exp3 exp3.c -L. -lrsm -lpthread

clean:
	rm -fr *.o *.a *~ a.out myapp app exp1 exp2 exp3 rsm.o rsm.a librsm.a
