#!/bin/csh

#number of threads:
foreach t (1 2 4 8)#6 8 10 16 24)
	#echo NUMT = $t
	foreach n (0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16)
	#	echo NUM = $n
			g++ -DNUM=$n -DNUMT=$t main.cpp -o main -lm -fopenmp
			./main
	end
end
