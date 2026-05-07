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
    unsigned int seed;

    // Método auxiliar para generar nombres de archivo basados en la configuración
    std::string generateFileName(std::string prefix, int sync, int sched, int chunk);

    public:
    Benchmark(int reps = 10, int steps = 1000, double dt = 0.01, unsigned int s = 123);

    simulation_data runScalabilityTest(
        int max_threads, 
        int num_particles, 
        int sync_type, 
        int schedule_type, 
        int chunk_size, 
        double G, 
        double epsilon
    );
    void setupRandomSystem(NBodySystem& system, int n);

};
#endif