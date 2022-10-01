#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

#include "../includes/config.h"
#include "md4.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s HASH\n", argv[0]);
    return -1;
  }
  unsigned char *target = parse_hash(argv[1]);
  char *candidate = malloc(PWD_LEN + 1);
  memset(candidate, '!', PWD_LEN);
  candidate[PWD_LEN] = 0;
  size_t tested = 0;
  struct timeval tval;
  double start;
  double now;

  gettimeofday(&tval, NULL);
  start = tval.tv_sec + tval.tv_usec / 1000000.0;

  do {
    MD4_CTX ctx;
    unsigned char res[16];
    MD4_Init(&ctx);
    MD4_Update(&ctx, candidate, PWD_LEN);
    MD4_Final(res, &ctx);
    tested++;
    if (memcmp(res, target, 16) == 0) {
      printf("found: %s, after %ld tries\n", candidate, tested);
      return 0;
    }
    if (tested % (1024 * 1024 * 32) == 0) {
      gettimeofday(&tval, NULL);
      now = tval.tv_sec + tval.tv_usec / 1000000.0;
      double speed = tested / (now - start);
      fprintf(stderr, "%.3f M/s\n", speed / 1000000.0);
    }
  } while (incr_candidate(candidate));
  printf("not found after %ld tries\n", tested);
  return 1;
}