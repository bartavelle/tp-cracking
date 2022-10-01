CFLAGS = -O3 -march=native -Wall

all: target/simple target/simd

target:
	mkdir target

target/simple: target simple-c/main.c simple-c/md4.c simple-c/md4.h
	gcc -o target/simple $(CFLAGS) simple-c/main.c simple-c/md4.c 

target/simd: target simd-c/main.c
	gcc -o target/simd $(CFLAGS) simd-c/main.c