#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <cmath>

#include "gauss.h"

struct gauss_state {
    std::mt19937 *gen;
    std::normal_distribution<> *dist;
    gauss_state(double mean, double stddev) {
        std::random_device rd;
        gen = new std::mt19937(rd());
        dist = new std::normal_distribution<>(mean, stddev);
    }
    ~gauss_state() {
        delete gen;
        delete dist;
    }
    double get(void) {
        return dist->operator()(*gen);
    }

};

void *gauss_create(double mean, double stddev)
{
    return (void*)new gauss_state(mean, stddev);
}

void gauss_destroy(void *g) {
    struct gauss_state *gs = (struct gauss_state*)g;
    delete gs;
}

double gauss(void *g)
{
    struct gauss_state *gs = (struct gauss_state*)g;
    return gs->get();
}
