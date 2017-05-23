all: main loopdetect

main: symengine.o varmap.o
	g++ -std=c++11 -Wall -g main.cpp symengine.o varmap.o -o llse

loopdetect:
	g++ -std=c++11 -Wall -g loopdetect.cpp -o loopdetect

symengine.o:
	g++ -c -std=c++11 -Wall -g symengine.cpp

varmap.o:
	g++ -c -std=c++11 -Wall -g varmap.cpp

clean:
	rm -f loopid symengine.o llse loopdetect varmap.o
