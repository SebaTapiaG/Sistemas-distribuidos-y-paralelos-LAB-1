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
//Estructuras para Pruebas del laboratorio 2
struct MeasurementResult {
    double mean_ms;      // T_barra (tiempo medio en ms)
    double stddev_ms;    // sigma_T (desviación estándar en ms)
};

struct CpuGpuComparison {
    int num_particles;
    int variant;
    int block_size;
    
    MeasurementResult cpu_serial;      // Baseline CPU Serial
    MeasurementResult gpu_kernel;      // GPU Kernel Only
    MeasurementResult gpu_end_to_end;  // GPU End-to-End
    
    double speedup_kernel;  // T_cpu / T_gpu_kernel
    double speedup_e2e;     // T_cpu / T_gpu_e2e
    double amdahl_f;        // Fracción serial estimada (Amdahl)
};

class Benchmark {
private:
    NBodySystem base_system;
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

    // Sobrecarga/Constructor para Lab 2 (GPU Benchmark con NBodySystem)
    Benchmark(const NBodySystem& system, int num_steps = 1000, double delta_t = 0.01, int reps = 10);

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

    // Métodos auxiliares de estadística
    static MeasurementResult calculateStats(const std::vector<double>& times_ms);

    // Mediciones Individuales
    MeasurementResult benchmarkCpuSerial(int runs = 10);

    MeasurementResult benchmarkKernelOnly(
        NBodySimulator& simulator, 
        int variant = 0, 
        int energy_method = 0, 
        int block_size = 256, 
        double* d_u_ptr = nullptr, 
        double* d_k_ptr = nullptr, 
        int runs = 10
    );

    MeasurementResult benchmarkEndToEnd(
        int variant = 0, 
        int energy_method = 0, 
        int block_size = 256, 
        int runs = 10
    );
    // Métodos para exportar y formatear resultados GPU
    static void writeGpuHeader(const std::string& filename);
    static void saveGpuComparison(const std::string& filename, const CpuGpuComparison& res);

    // Orquestador Principal que ejecuta y guarda la comparación
    CpuGpuComparison runGpuComparisonTest(
        const std::string& outputFile,
        int num_particles,
        int variant = 0,
        int block_size = 256,
        int energy_method = 0,
        int runs = 10
    );
    // Método para ejecutar la suite completa de 40 pruebas CPU vs GPU (Kernel y E2E)
    void runFullGpuTestSuite(const std::string& outputFile, int runs = 5);
    };

#endif