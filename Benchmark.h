#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <vector>
#include <string>
#include "NBodySimulator.h"
#include "MetricsCalculator.h"

class Benchmark {
private:
    int repetitions;
    int steps;
    double dt;
    unsigned int seed;

    std::string generateFileName(std::string prefix, int sync_type, int task_type, int energy_method, int schedule_type, int chunk_size);

public:
    Benchmark(int reps = 10, int steps = 1000, double dt = 0.01, unsigned int s = 123);

    // Versión modificada: Ahora es el centro de mando
    simulation_data runScalabilityTest(
        int max_threads, 
        int num_particles, 
        int task_type,      // Nuevo: 0=Task, 1=Parallel For
        int sync_type,      // 0=atomic, 1=critical, 2=nowait
        int energy_method,  // Nuevo: 0=reduce, 1=atomic
        int schedule_type, 
        int chunk_size, 
        double G, 
        double epsilon,
        bool perform_diagnostics = false // Nuevo: Controla si se ejecuta la fase de diagnóstico
    );

    void setupRandomSystem(NBodySystem& system, int n);
};

#endif