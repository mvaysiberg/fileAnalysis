all: Asst2.c structures.o parse.o
	gcc Asst2.c -o detector -lm
structures.o: structures.c structures.h
	gcc -c structures.c
parse.o: parse.c parse.h
	gcc -c parse.c
clean:
	rm detector
	rm *.o