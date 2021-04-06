all:
	g++ -std=c++11 -o a.out col216a4.cpp

.SILENT:
	all
clean:
	rm -f ./a.out