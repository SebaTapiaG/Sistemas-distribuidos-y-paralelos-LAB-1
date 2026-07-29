#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <vector>
#include <string>
#include "NBodySimulator.h"
#include "MetricsCalculator.h"
struct PerformanceResult {
    double mean_time;
    double std_dev;
    double speedup;
    double serial_fraction; // f en la Ley de Amdahl
    double theorical_serial_fraction; // f estimada por tiempo
    double sigma_serial_fraction;
    double theorical_speedup;
    double sigma_theorical_speedup;
    double sigma_speedup;
    double efficiency;
    double sigma_efficiency;

};
class Benchmark {
private:
    int repetitions;
    int steps;
    double dt;
    unsigned int seed;

    std::string generateFileName(std::string prefix, int sync_type, int task_type, int energy_method, int schedule_type, int chunk_size);

public:
    static PerformanceResult analyzePerformance(const std::vector<double>& t1_times, 
                                            const std::vector<double>& tp_times,
                                            int num_threads);
    // Análisis Teórico
    static double estimateSerialFraction(double speedup, int p);
    static double estimateSerialFractionByTime(double t1, double tp, int p);

    static double calculateTheoricalSpeedup(double f, int p);
    static PerformanceResult analyzePerformanceFirstPrivate(const std::vector<double>& t1_times, 
                                            const std::vector<double>& tp_times,
                                            const std::vector<double>& ts_times,
                                            int num_threads);
    //Sobrecarga para seleccionar entre métodos de análisis (ej: sin firstprivate (0), con firstprivate (1))
    static PerformanceResult analyzePerformance(const std::vector<double>& t1_times, 
                                            const std::vector<double>& tp_times, 
                                            const std::vector<double>& ts_times,
                                            int num_threads, int mode);        


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
        bool perform_diagnostics = false, // Nuevo: Controla si se ejecuta la fase de diagnóstico
        int mode = 0 // Nuevo: Selecciona el método de análisis de rendimiento (0=sin firstprivate, 1=con firstprivate)
    );

    void setupRandomSystem(NBodySystem& system, int n);
};

#endif