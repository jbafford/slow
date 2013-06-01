all: slow
	

slow: src/slow.o
	gcc -o $@ $<

slow.o: src/slow.c
	gcc -c -o $@ $<

clean:
	rm -f slow src/*.o
