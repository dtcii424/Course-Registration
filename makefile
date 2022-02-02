all:
	gcc -g -c -o cs1713p5Driver.o cs1713p5Driver.c
	gcc -g -c -o cs1713p5.o cs1713p5.c
	gcc -g -o register cs1713p5Driver.o cs1713p5.o
clean:
	rm -f *.o
run:
	./register -s p5Courses.txt -c p5Command.txt

