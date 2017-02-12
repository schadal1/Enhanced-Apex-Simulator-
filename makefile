all: run

run:project2.c
	gcc project2.c -o project2
	./project2
clean:
	rm -f project2 .*~
