default: all
all:a3chat
a3chat:a3chat.c
	gcc -pthread -o a3chat a3chat.c
clean:
	rm -f a3chat
	rm -f a3chat.o
	rm -f *.in
	rm -f *.out
tar:
	tar zcvf submit.tar *