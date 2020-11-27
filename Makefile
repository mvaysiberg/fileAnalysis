all: Asst2.c structures.o 
	gcc Asst2.c -o detector -lm
structures.o: structures.c structures.h
	gcc -c structures.c
clean:
	rm detector