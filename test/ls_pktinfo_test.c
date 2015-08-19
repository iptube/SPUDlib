/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include "ls_pktinfo.h"
#include "test_utils.h"

CTEST_DATA(ls_pktinfo)
{
  ls_pktinfo* pi;
  ls_err      err;
};

CTEST_SETUP(ls_pktinfo)
{
  ASSERT_TRUE( ls_pktinfo_create(&data->pi, &data->err) );
}

CTEST_TEARDOWN(ls_pktinfo)
{
  ls_pktinfo_destroy(data->pi);
}

CTEST2(ls_pktinfo, dup_fail)
{
  ls_pktinfo* pi = NULL;
  OOM_RECORD_ALLOCS( ls_pktinfo_dup(data->pi, &pi, &data->err) );
  ls_pktinfo_destroy(pi);
  OOM_TEST_INIT()
  OOM_TEST_CONDITIONAL_CHECK(&data->err,
                             ls_pktinfo_dup(data->pi, &pi, &data->err), true);
}

CTEST2(ls_pktinfo, dup)
{
  ls_pktinfo* pi;
  ASSERT_TRUE( ls_pktinfo_dup(data->pi, &pi, &data->err) );
  ls_pktinfo_destroy(pi);
}

CTEST2(ls_pktinfo, clear)
{
  ls_pktinfo_clear(data->pi);
}

CTEST2(ls_pktinfo, set4)
{
  struct in_pktinfo info;
  ASSERT_FALSE( ls_pktinfo_is_full(data->pi) );
  ls_pktinfo_set4(data->pi, &info);
  ASSERT_TRUE( ls_pktinfo_is_full(data->pi) );
}

CTEST2(ls_pktinfo, set6)
{
  struct in6_pktinfo info;
  ASSERT_FALSE( ls_pktinfo_is_full(data->pi) );
  ls_pktinfo_set6(data->pi, &info);
  ASSERT_TRUE( ls_pktinfo_is_full(data->pi) );
}

CTEST2(ls_pktinfo, get_addr)
{
  struct in_pktinfo       info4;
  struct in6_pktinfo      info6;
  struct sockaddr_storage ss;
  socklen_t               ss_sz = 0;

  ASSERT_NULL( ls_pktinfo_get_info(data->pi) );
  ASSERT_FALSE( ls_pktinfo_get_addr(data->pi, (struct sockaddr*)&ss, &ss_sz,
                                    &data->err) );

  info4.ipi_addr.s_addr = htonl(0x7f000001);
  ls_pktinfo_set4(data->pi, &info4);
  ASSERT_FALSE( ls_pktinfo_get_addr(data->pi, (struct sockaddr*)&ss, &ss_sz,
                                    &data->err) );
  ASSERT_EQUAL(0, ss_sz);
  ss_sz = sizeof(ss);
  ASSERT_TRUE( ls_pktinfo_get_addr(data->pi, (struct sockaddr*)&ss, &ss_sz,
                                   &data->err) );
  ASSERT_EQUAL(AF_INET,                    ss.ss_family);
  ASSERT_EQUAL(sizeof(struct sockaddr_in), ss_sz);
  ASSERT_NOT_NULL( ls_pktinfo_get_info(data->pi) );

  info6.ipi6_addr = in6addr_any;
  ls_pktinfo_set6(data->pi, &info6);

  ss_sz = 0;
  ASSERT_FALSE( ls_pktinfo_get_addr(data->pi, (struct sockaddr*)&ss, &ss_sz,
                                    &data->err) );

  ss_sz = sizeof(ss);
  ASSERT_TRUE( ls_pktinfo_get_addr(data->pi, (struct sockaddr*)&ss, &ss_sz,
                                   &data->err) );
  ASSERT_EQUAL(AF_INET6,                    ss.ss_family);
  ASSERT_EQUAL(sizeof(struct sockaddr_in6), ss_sz);
  ASSERT_NOT_NULL( ls_pktinfo_get_info(data->pi) );
}

CTEST2(ls_pktinfo, cmsg)
{
  char               buf[32];
  struct in_pktinfo  info4;
  struct in6_pktinfo info6;

  ASSERT_EQUAL( 0, ls_pktinfo_cmsg(data->pi, (struct cmsghdr*)buf) );

  ls_pktinfo_set4(data->pi, &info4);
  ASSERT_NOT_EQUAL( 0, ls_pktinfo_cmsg(data->pi, (struct cmsghdr*)buf) );

  ls_pktinfo_set6(data->pi, &info6);
  ASSERT_NOT_EQUAL( 0, ls_pktinfo_cmsg(data->pi, (struct cmsghdr*)buf) );
}
