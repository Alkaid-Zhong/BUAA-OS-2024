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
chmod 655 err.txt
x=1
y=1
if [ $# -gt 0 ]
then
	x=$[$1]
fi
if [ $# -gt 1 ]
then
	y=$[$2]
fi
n=$[$x+$y]p
sed -n $n  err.txt >&2

