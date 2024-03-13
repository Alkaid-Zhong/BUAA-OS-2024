mkdir test
cp -r ./code ./test
cat ./code/14.c
a=0
while [ $a -lt 16 ]
do
	gcc -c ./test/code/$a.c -o ./test/code/$a.o
	a=$[$a+1]
done
gcc ./test/code/*.o -o ./test/hello
./test/hello 2> ./test/err.txt
mv ./test/err.txt ./
chmod -rw -r -xr -x  err.txt

