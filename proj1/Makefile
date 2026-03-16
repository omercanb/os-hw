all: findlwcheck findlwp findlwt
findlwcheck: findlwcheck.c findlw.c buffer.c
	gcc -Wall -ggdb -o findlwcheck findlwcheck.c
findlwp: findlwp.c findlw.c buffer.c
	gcc -Wall -ggdb -o findlwp findlwp.c
findlwt: findlwt.c findlw.c buffer.c
	gcc -Wall -ggdb -o findlwt findlwt.c
clean:
	rm -fr findlw findlw.o *~
