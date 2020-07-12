# makefile para littleC, versión 1
# usar tabulador (no espacios) en la línea de comando

littlec : little.o lclib.o parser.o
	gcc -o littlec little.o lclib.o parser.o

little.o: src/little.c
	gcc -c src/little.c
lclib.o: src/lclib.c
	gcc -c src/lclib.c
parser.o: src/parser.c
	gcc -c src/parser.c

clean:
	rm *.o