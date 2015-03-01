#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void *gauss_create(double mean, double stddev);
void gauss_destroy(void *g);
double gauss(void *g);

#ifdef __cplusplus
}
#endif
