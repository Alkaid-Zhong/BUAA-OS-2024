all:
	gcc -I./include ./code/fibo.c -c -o ./code/fibo.o
	gcc -I./include ./code/main.c -c -o ./code/main.o
	gcc ./code/fibo.o ./code/main.o -o fibo
clean:
	rm -rf ./code/*.o
.PHONY: all clean
