all: sender reciever

sender: sender.c libsock.a
	gcc -o sender sender.c libsock.a -Wall -Werror -pthread -lrt -g

reciever: reciever.c libsock.a
	gcc -o reciever reciever.c libsock.a -Wall -Werror -pthread -lrt -g

libsock.a: hashtable.o socketutil.o
	ar -r libsock.a hashtable.o socketutil.o 
hashtable.o: hashtable.c hashtable.h
	gcc -c hashtable.c hashtable.h -Wall -Werror

socketutil.o: socketutil.c socketutil.h
	gcc -c socketutil.c socketutil.h -Wall -Werror  -pthread -g

clean:
	rm -r -f Makefile~ reciever.c~ sender.c~ socketutil.c~ socketutil.h~ socketutil.o libsock.a hashtable.o test2.txt test2.txt~ sender reciever socketutil.h.gch test.txt~