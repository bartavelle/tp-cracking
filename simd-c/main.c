#include <stdio.h>

#include "../includes/config.h"

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s HASH\n", argv[0]);
    return -1;
  }
  unsigned char *target = parse_hash(argv[1]);
  char *candidate = malloc(PWD_LEN + 1);
  memset(candidate, '!', PWD_LEN);
  candidate[PWD_LEN] = 0;
  printf("TODO\n");
}