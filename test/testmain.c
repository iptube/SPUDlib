/*
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "ls_log.h"

Suite * spud_suite (void);
Suite * tube_suite (void);
Suite * ls_str_suite (void);
Suite * ls_sockaddr_suite (void);
Suite * ls_error_suite (void);
Suite * ls_mem_suite (void);
Suite * ls_log_suite (void);
Suite * ls_htable_suite (void);
Suite * ls_eventing_suite (void);
Suite * cbor_suite (void);

int main(void){

    int number_failed;
    Suite *s = spud_suite ();
    SRunner *sr = srunner_create (s);

    ls_log_set_level(LS_LOG_ERROR);
    srunner_add_suite (sr,  tube_suite () );
    srunner_add_suite (sr,  ls_str_suite () );
    srunner_add_suite (sr,  ls_sockaddr_suite () );
    srunner_add_suite (sr,  ls_error_suite () );
    srunner_add_suite (sr,  ls_mem_suite () );
    srunner_add_suite (sr,  ls_log_suite () );
    srunner_add_suite (sr,  ls_htable_suite () );
    srunner_add_suite (sr,  ls_eventing_suite () );
    srunner_add_suite (sr,  cbor_suite () );
    srunner_run_all (sr, CK_NORMAL);
    number_failed = srunner_ntests_failed (sr);
    srunner_free (sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;


}
