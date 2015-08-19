/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include "ls_str.h"

#include <stdlib.h>
#include <string.h>

/****************************************************************************
 * EXTERNAL functions
 */

LS_API int
ls_atoi(const char* a,
        int         def)
{
  if (a == NULL)
  {
    return def;
  }
  return atoi(a);
}

LS_API size_t
ls_strlen(const char* a)
{
  if (a == NULL)
  {
    return 0;
  }
  return strlen(a);
}

LS_API size_t
ls_strnlen(const char* a,
           size_t      len)
{
  size_t i;
  if (a == NULL)
  {
    return 0;
  }
  for (i = 0; i < len && a[i]; i++)
  {
    /* no-op */
  }
  return i;
}

LS_API int
ls_strcmp(const char* a,
          const char* b)
{
  if (a == b)
  {
    return 0;
  }
  if (a == NULL)
  {
    return -1;
  }
  if (b == NULL)
  {
    return 1;
  }
  return strcmp(a, b);
}

LS_API int
ls_strcasecmp(const char* a,
              const char* b)
{
  if (a == b)
  {
    return 0;
  }
  if (a == NULL)
  {
    return -1;
  }
  if (b == NULL)
  {
    return 1;
  }
  return strcasecmp(a, b);
}

LS_API int
ls_strncmp(const char* a,
           const char* b,
           size_t      n)
{
  if (a == b)
  {
    return 0;
  }
  if (a == NULL)
  {
    return -1;
  }
  if (b == NULL)
  {
    return 1;
  }
  return strncmp(a, b, n);
}

LS_API int
ls_strncasecmp(const char* a,
               const char* b,
               size_t      n)
{
  if (a == b)
  {
    return 0;
  }
  if (a == NULL)
  {
    return -1;
  }
  if (b == NULL)
  {
    return 1;
  }
  return strncasecmp(a, b, n);
}
