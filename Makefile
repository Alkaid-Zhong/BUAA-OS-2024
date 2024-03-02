.PHONY: clean

out: calc case_all
# Your code here.
compile:
	gcc casegen.c -o casegen
case_add: compile
	./casegen add 100 > case_add
case_sub: compile
	./casegen sub 100 > case_sub
case_mul: compile
	./casegen mul 100 > case_mul
case_div: compile
	./casegen div 100 > case_div
case_all: case_add case_sub case_mul case_div
	cat case_add case_sub case_mul case_div > case_all
calc: case_all
	gcc calc.c -o calc
	./calc < case_all > out
clean:
	rm -f out calc casegen case_* *.o
