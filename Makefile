all: mli
mli: main.o scanner.o parser.o tables.o error.o executer.o
	g++ -o mli main.o scanner.o parser.o tables.o error.o executer.o
main.o : main.cpp
	g++ -c -g -Wall -pedantic -ansi main.cpp
scanner.o: scanner.cpp
	g++ -c -g -Wall -pedantic -ansi scanner.cpp
parser.o: parser.cpp
	g++ -c -g -Wall -pedantic -ansi parser.cpp
tables.o: tables.cpp
	g++ -c -g -Wall -pedantic -ansi tables.cpp
error.o: error.cpp
	g++ -c -g -Wall -pedantic -ansi error.cpp
executer.o: executer.cpp
	g++ -c -g -Wall -pedantic -ansi executer.cpp
clean:
	rm -f main.o scanner.o parser.o tables.o error.o executer.o mli
