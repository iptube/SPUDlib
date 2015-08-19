/**
 * \file
 * \brief
 * Internal logging functions, not for use outside library and unit tests.
 *
 * Copyright (c) 2015 SPUDlib authors.  See LICENSE file.
 */

#pragma once

#include "ls_mem.h"

/**
 * Set the allocator and deallocator used for log messages.
 *
 * If used, this function should be called before any other log functions are
 * called.
 *
 * \param allocator the allocator to use (pass NULL to use the default:
 *   ls_data_malloc)
 * \param deallocator the deallocator to use (pass NULL to use the default:
 *   ls_data_free)
 */
void
_ls_log_set_memory_funcs(ls_data_malloc_func allocator,
                         ls_data_free_func   deallocator);
