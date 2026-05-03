#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <vector>
#include <string>
#include "NBodySimulator.h"
#include "MetricsCalculator.h"
#include "Visualizer.h"

class Benchmark {
private:
    int repetitions;
    int steps;
    double dt;
    unsigned int seed; // Semilla inyectada

public:
    // Constructor con semilla
    Benchmark(int reps = 10, int steps = 1000, double dt = 0.01, unsigned int s = 123);

    // Ejecuta el test con un tipo de sincronización específico
    // sync_type: 0=atomic, 1=critical, etc. (según NBodySimulator)
    void runScalabilityTest(int max_threads, int num_particles, int sync_type);

private:
    void setupRandomSystem(NBodySystem& system, int n);
};

#endif