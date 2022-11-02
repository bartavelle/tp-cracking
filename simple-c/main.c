#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

#include "../includes/config.h"
#include "md4.h"

int hash_compare(const unsigned char *target, const char *candidate)
{
  MD4_CTX ctx;
  // init

  ctx.A = 0x67452301;
  ctx.B = 0xefcdab89;
  ctx.C = 0x98badcfe;
  ctx.D = 0x10325476;

  ctx.lo = 0;
  ctx.hi = 0;

  // update
  unsigned long used;

  unsigned long size = PWD_LEN;
  const void *data = candidate;

  ctx.lo = PWD_LEN;

  memset(ctx.buffer, 0, 64);
  memcpy(ctx.buffer, data, size);

  // final
  used = PWD_LEN;
  ctx.buffer[PWD_LEN] = 0x80;
  used++;

  ctx.lo = PWD_LEN * 8;
  ctx.buffer[56] = ctx.lo;

  body(&ctx, ctx.buffer, 64);

  unsigned char result[16];
  result[0] = ctx.A;
  result[1] = ctx.A >> 8;
  result[2] = ctx.A >> 16;
  result[3] = ctx.A >> 24;
  result[4] = ctx.B;
  result[5] = ctx.B >> 8;
  result[6] = ctx.B >> 16;
  result[7] = ctx.B >> 24;
  result[8] = ctx.C;
  result[9] = ctx.C >> 8;
  result[10] = ctx.C >> 16;
  result[11] = ctx.C >> 24;
  result[12] = ctx.D;
  result[13] = ctx.D >> 8;
  result[14] = ctx.D >> 16;
  result[15] = ctx.D >> 24;

  return memcmp(result, target, 16);
}

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
  struct timeval tval;
  double start;
  double now;

  gettimeofday(&tval, NULL);
  start = tval.tv_sec + tval.tv_usec / 1000000.0;

  do
  {
    tested++;
    // MD4_CTX ctx;
    // unsigned char res[16];
    // MD4_Init(&ctx);
    // MD4_Update(&ctx, candidate, PWD_LEN);
    // MD4_Final(res, &ctx);

    // if (memcmp(res, target, 16) == 0)
    if (hash_compare(target, candidate) == 0)
    {
      printf("found: %s, after %ld tries\n", candidate, tested);
      return 0;
    }
    if (tested % (1024 * 1024 * 32) == 0)
    {
      gettimeofday(&tval, NULL);
      now = tval.tv_sec + tval.tv_usec / 1000000.0;
      double speed = tested / (now - start);
      fprintf(stderr, "%.3f M/s\n", speed / 1000000.0);
    }
  } while (incr_candidate(candidate));
  printf("not found after %ld tries\n", tested);
  return 1;
}