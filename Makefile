all: raycast.c
	gcc raycast.c -o raycast

clean:
	rm -rf raycast *~

test:
	./raycast 100 100 input.json output.ppm
