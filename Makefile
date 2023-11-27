CFLAGS = -O3 -march=native -Wall

all: target target/simple target/simd target/gpu

target:
	mkdir target

target/simple: simple-c/main.c simple-c/md4.c simple-c/md4.h includes/parse.h includes/config.h
	gcc -o target/simple $(CFLAGS) simple-c/main.c simple-c/md4.c 

target/simd: simd-c/main.c includes/parse.h includes/config.h
	gcc -o target/simd $(CFLAGS) simd-c/main.c

target/gpu: gpu-opencl/main.c gpu-opencl/vector_md4_kernel.cl includes/parse.h includes/config.h
	clang-15 --std=cl2.0 -S -emit-llvm -o /dev/null \
		 -Xclang -finclude-default-header gpu-opencl/vector_md4_kernel.cl \
	 && gcc -Wall -Igpu-opencl -o target/gpu -Wall gpu-opencl/main.c -l OpenCL
