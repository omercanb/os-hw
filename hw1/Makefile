all: wlf
myprog: wlf.c
	gcc -Wall -g -o wlf wlf.c
myprog2: wlf2.c
	gcc -Wall -g -o wlf wlf2.c
test: dll.c dll_test.c
	gcc -Wall -g -o test dll_test.c
clean:
	rm -fr wlf wlf.o *~
