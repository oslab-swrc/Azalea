all: imagemaker qimagemaker

imagemaker: imagemaker.c
	gcc -g -o $@ $<
uthread-imagemaker: uthread-imagemaker.c
	gcc -g -o $@ $<
qimagemaker: qimagemaker.c
	gcc -g -o $@ $<
clean:
	rm -f imagemaker uthread-imagemaker qimagemaker

