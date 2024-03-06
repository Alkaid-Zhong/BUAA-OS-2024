all:
	make -C code
	gcc ./code/fibo.o ./code/main.o -o fibo
clean:
	rm -rf ./code/*.o
.PHONY: all clean
