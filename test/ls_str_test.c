/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include "test_utils.h"
#include "../src/ls_str.h"


CTEST(ls_str, atoi)
{
  char num[] = "12";

  ASSERT_TRUE(ls_atoi(num, 3) == 12);
  ASSERT_TRUE(ls_atoi(NULL, 3) == 3);

}

CTEST(ls_str, strlen)
{
  char test[] = "ABCDEFG";

  ASSERT_TRUE(ls_strlen(NULL) == 0);
  ASSERT_TRUE(ls_strlen(test) == 7);
}

CTEST(ls_str, strnlen)
{
  char test[] = "ABCDEFG";

  ASSERT_TRUE(ls_strnlen(NULL, 12) == 0);
  ASSERT_TRUE(ls_strnlen(test, 12) == 7);
}

CTEST(ls_str, strcmp)
{
  char a[]  = "ABCDEFG";
  char aa[] = "ABCDEFG";
  char b[]  = "ABCDEFGH";
  char bb[] = "ABCDEFGH";

  ASSERT_TRUE(ls_strcmp(a,a) == 0);
  ASSERT_TRUE(ls_strcmp(b,b) == 0);
  ASSERT_TRUE(ls_strcmp(a,aa) == 0);
  ASSERT_TRUE(ls_strcmp(b,bb) == 0);
  ASSERT_FALSE(ls_strcmp(a,b) == 0);
  ASSERT_FALSE(ls_strcmp(aa,b) == 0);
  ASSERT_FALSE(ls_strcmp(aa,bb) == 0);
}

CTEST(ls_str, strcasecmp)
{
  char a[]  = "ABCDEFG";
  char aa[] = "abcdefg";
  char b[]  = "ABCDEFGH";
  char bb[] = "abcdefgh";

  ASSERT_TRUE(ls_strcasecmp(a,a) == 0);
  ASSERT_TRUE(ls_strcasecmp(b,b) == 0);
  ASSERT_TRUE(ls_strcasecmp(a,aa) == 0);
  ASSERT_TRUE(ls_strcasecmp(b,bb) == 0);
  ASSERT_FALSE(ls_strcasecmp(a,b) == 0);
  ASSERT_FALSE(ls_strcasecmp(aa,b) == 0);
  ASSERT_FALSE(ls_strcasecmp(aa,bb) == 0);
}

CTEST(ls_str, strncmp)
{
  char a[]  = "ABCDEFG";
  char aa[] = "ABCDEFG";
  char b[]  = "ABCDEFGH";
  char bb[] = "ABCDEFGH";

  ASSERT_TRUE(ls_strncmp(a,a,12) == 0);
  ASSERT_TRUE(ls_strncmp(b,b,3) == 0);
  ASSERT_TRUE(ls_strncmp(a,aa,15) == 0);
  ASSERT_TRUE(ls_strncmp(b,bb,4) == 0);
  ASSERT_FALSE(ls_strncmp(a,b,17) == 0);
  ASSERT_FALSE(ls_strncmp(aa,b,34) == 0);
  ASSERT_FALSE(ls_strncmp(aa,bb,12) == 0);
}

CTEST(ls_str, atoi_test)
{
  ASSERT_EQUAL(ls_atoi("24", 0),  24);
  ASSERT_EQUAL(ls_atoi("-42", 0), -42);
  ASSERT_EQUAL(ls_atoi("", 0),    0);
  ASSERT_EQUAL(ls_atoi(NULL, 0),  0);

  ASSERT_EQUAL(ls_atoi("24", 5),  24);
  ASSERT_EQUAL(ls_atoi("-42", 5), -42);
  ASSERT_EQUAL(ls_atoi("", 5),    0);
  ASSERT_EQUAL(ls_atoi(NULL, 5),  5);
}

CTEST(ls_str, compare_test)
{
  char* str1, * str2;

  str1 = str2 = "a test string";
  ASSERT_EQUAL(ls_strcmp(str1, str2),     0);
  ASSERT_EQUAL(ls_strcasecmp(str1, str2), 0);
  str2 = "b test string";
  ASSERT_TRUE(ls_strcmp(str1, str2) < 0);
  ASSERT_TRUE(ls_strcasecmp(str1, str2) < 0);
  str1 = "c test";
  ASSERT_TRUE(ls_strcmp(str1, str2) > 0);
  ASSERT_TRUE(ls_strcasecmp(str1, str2) > 0);

  str1 = "a test string";
  str2 = "A test String";
  ASSERT_TRUE(ls_strcmp(str1, str2) > 0);
  ASSERT_EQUAL(ls_strcasecmp(str1, str2), 0);

  ASSERT_EQUAL(ls_strcmp(NULL, NULL),     0);
  ASSERT_EQUAL(ls_strcasecmp(NULL, NULL), 0);

  ASSERT_TRUE(ls_strcmp(str1, NULL) > 0);
  ASSERT_TRUE(ls_strcasecmp(str1, NULL) > 0);

  ASSERT_TRUE(ls_strcmp(NULL, str2) < 0);
  ASSERT_TRUE(ls_strcasecmp(NULL, str2) < 0);
}

CTEST(ls_str, ncompare_test)
{
  char* str1, * str2, * str2case;

  str1     = str2 = "test string alpha";
  str2case = "Test String AlphA";
  ASSERT_EQUAL(ls_strncmp(str1, str2, 17),         0);
  ASSERT_EQUAL(ls_strncasecmp(str1, str2, 17),     0);
  ASSERT_EQUAL(ls_strncasecmp(str1, str2case, 17), 0);

  str2     = "test string beta";
  str2case = "Test String BetA";
  ASSERT_TRUE(ls_strncmp(str1, str2, 17) < 0);
  ASSERT_TRUE(ls_strncasecmp(str1, str2case, 17) < 0);

  str2     = "test string al";
  str2case = "Test String Al";
  ASSERT_TRUE(ls_strncmp(str1, str2, 17) > 0);
  ASSERT_TRUE(ls_strncmp(str1, str2case, 17) > 0);

  ASSERT_EQUAL(ls_strncmp(NULL, NULL, 10),     0);
  ASSERT_EQUAL(ls_strncasecmp(NULL, NULL, 10), 0);
  ASSERT_TRUE(ls_strncmp(str1, NULL, 10) > 0);
  ASSERT_TRUE(ls_strncasecmp(str1, NULL, 10) > 0);
  ASSERT_TRUE(ls_strncmp(NULL, str2, 10) < 0);
  ASSERT_TRUE(ls_strncasecmp(NULL, str2, 10) < 0);
}

CTEST(ls_str, length_test)
{
  char* str;

  str = "a test string";
  ASSERT_EQUAL(ls_strlen(str),      13);

  str = "";
  ASSERT_EQUAL(ls_strlen(str),      0);

  str = "another test string";
  ASSERT_EQUAL(ls_strlen(str),      19);

  ASSERT_EQUAL(ls_strlen(NULL),     0);

  str = "a test string";
  ASSERT_EQUAL(ls_strnlen(str, 13), 13);

  str = "a test string";
  ASSERT_EQUAL(ls_strnlen(str, 17), 13);

  str = "";
  ASSERT_EQUAL(ls_strnlen(str, 4),  0);

  str = "another test string";
  ASSERT_EQUAL(ls_strnlen(str, 12), 12);

  ASSERT_EQUAL(ls_strnlen(NULL, 4), 0);
}
