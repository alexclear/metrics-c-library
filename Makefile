test: main.o prometheus.o
	gcc -g main.o prometheus.o -lglib-2.0 -o test

main.o:
	gcc -g -c main.c

prometheus.o:
	gcc -g -c prometheus.c -I/usr/include/glib-2.0/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/

clean:
	rm -rf *.o
	rm test
