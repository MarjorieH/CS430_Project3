all: raycast.c
	gcc raycast.c -o raycast

clean:
	rm -rf raycast *~

test:
	./raycast 400 400 input.json output.ppm
