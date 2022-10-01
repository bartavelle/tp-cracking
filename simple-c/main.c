#include <stdio.h>
#include "../includes/config.h"
#include "md4.h"

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
  size_t tested = 0;
  do
  {
    MD4_CTX ctx;
    unsigned char res[16];
    MD4_Init(&ctx);
    MD4_Update(&ctx, candidate, PWD_LEN);
    MD4_Final(res, &ctx);
    tested++;
    if (memcmp(res, target, 16) == 0)
    {
      printf("found: %s, after %ld tries\n", candidate, tested);
      return 0;
    }
  } while (incr_candidate(candidate));
  printf("not found after %ld tries\n", tested);
  return 1;
}