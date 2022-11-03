#ifndef __CONFIG_H
#define CONFIG_H

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "parse.h"

#define PWD_LEN 6

int incr_candidate(char *ptr)
{
  ssize_t pos = PWD_LEN - 1;
  while (1)
  {
    if (pos < 0)
    {
      return 0;
    }
    char c = ++ptr[pos];
    if (c == '\'')
    {
      ptr[pos] = 'a';
      return 1;
    }
    if (c <= '&')
      return 1;
    if (c > 'z')
    {
      ptr[pos] = '!';
      pos--;
    } else {
      return 1;
    }
  }
}

#endif