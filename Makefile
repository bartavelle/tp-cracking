CFLAGS = -O3 -march=native -Wall

all: target target/simple target/simd target/gpu

target:
	mkdir target

target/simple: simple-c/main.c simple-c/md4.c simple-c/md4.h
	gcc -o target/simple $(CFLAGS) simple-c/main.c simple-c/md4.c 

target/simd: simd-c/main.c
	gcc -o target/simd $(CFLAGS) simd-c/main.c

target/gpu: gpu-opencl/main.c gpu-opencl/vector_md4_kernel.cl
	clang-12 --std=cl2.0 -S -emit-llvm -o /dev/null -Xclang -finclude-default-header gpu-opencl/vector_md4_kernel.cl && gcc -Wall -o target/gpu -Wall gpu-opencl/main.c -l OpenCL
