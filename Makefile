all: libprometheus.so.1.0

install: libprometheus.so.1.0
	install -m 0644 libprometheus.so.1.0 /usr/lib/libprometheus.so.1.0
	install -m 0644 prometheus.h /usr/include/prometheus.h
	test -L /usr/lib/libprometheus.so.1 && unlink /usr/lib/libprometheus.so.1
	ln -s /usr/lib/libprometheus.so.1.0 /usr/lib/libprometheus.so.1
	test -L /usr/lib/libprometheus.so && unlink /usr/lib/libprometheus.so
	ln -s /usr/lib/libprometheus.so.1 /usr/lib/libprometheus.so

libprometheus.so.1.0: prometheus.o
	gcc -shared -fPIC -Wl,-soname,libprometheus.so.1 -o libprometheus.so.1.0 ./prometheus.o -lglib-2.0

prometheus.o:
	gcc -g -c prometheus.c -I/usr/include/glib-2.0/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/ -fPIC

clean:
	rm -rf *.o
	rm libprometheus.so.1.0
