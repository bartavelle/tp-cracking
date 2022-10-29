#define PWD_LEN 7


__kernel void md4_crack(__global const unsigned int *target,
                        __global unsigned char *solution) {
  // Get the index of the current element to be processed
  int id = get_global_id(0);



}
