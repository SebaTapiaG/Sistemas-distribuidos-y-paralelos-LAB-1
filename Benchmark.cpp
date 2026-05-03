#include "Benchmark.h"
#include <omp.h>
#include <iostream>
#include <fstream>
#include <random>
#include <iomanip>

Benchmark::Benchmark(int reps, int s, double delta_t, unsigned int s_seed) 
    : repetitions(reps), steps(s), dt(delta_t), seed(s_seed) {}

void Benchmark::setupRandomSystem(NBodySystem& system, int n) {
    std::mt19937 gen(seed); 
    std::uniform_real_distribution<> pos_dis(0, 1000);
    std::uniform_real_distribution<> mass_dis(1, 100);

    for (int i = 0; i < n; ++i) {
        system.addParticle(Particle(mass_dis(gen), pos_dis(gen), pos_dis(gen)));
    }
}

void Benchmark::runScalabilityTest(int max_threads, int num_particles, int sync_type) {
    // Definición de archivos según enunciado
    std::ofstream resFile("benchmark_results.dat", std::ios::app);
    std::ofstream scalFile("scaling_analysis.dat", std::ios::app);
    
    // Si los archivos están vacíos, ponemos encabezados
    auto checkHeader = [](std::ofstream& f, std::string header) {
        f.seekp(0, std::ios::end);
        if (f.tellp() == 0) f << header << std::endl;
    };

    checkHeader(resFile, "# Sync | Threads | T_Sim_Avg | T_Sim_StdDev");
    checkHeader(scalFile, "# Sync | Threads | Speedup | Efficiency");

    // --- 1. EJECUCIÓN SERIAL PURA (T1) ---
    std::vector<double> t1_times;
    std::cout << "Midiendo referencia Serial (T1)..." << std::endl;
    for (int r = 0; r < repetitions; ++r) {
        NBodySystem system(1.0, 10.0);
        setupRandomSystem(system, num_particles);
        NBodySimulator sim(&system, dt);
        double start = omp_get_wtime();
        sim.processBodies(steps); 
        t1_times.push_back(omp_get_wtime() - start);
    }

    // --- 2. BUCLE DE HILOS (Tp) ---
    for (int p = 1; p <= max_threads; p *= 2) {
        std::vector<double> tp_times;
        std::cout << "Corriendo con " << p << " hilos..." << std::endl;
        omp_set_num_threads(p);

        for (int r = 0; r < repetitions; ++r) {
            NBodySystem system(1.0, 10.0);
            setupRandomSystem(system, num_particles);
            NBodySimulator sim(&system, dt);
            double start = omp_get_wtime();
            sim.processBodies(steps, sync_type);
            tp_times.push_back(omp_get_wtime() - start);
        }

        // Usamos MetricsCalculator para obtener el análisis completo
        PerformanceResult res = MetricsCalculator::analyzePerformance(t1_times, tp_times, p);

        // Guardar en benchmark_results.dat (Tiempos)
        resFile << std::left << std::setw(6) << sync_type 
                << std::setw(8) << p 
                << std::fixed << std::setprecision(6) << std::setw(15) << res.mean_time 
                << std::setw(15) << res.std_dev << std::endl;

        // Guardar en scaling_analysis.dat (Escalabilidad)
        scalFile << std::left << std::setw(6) << sync_type 
                 << std::setw(8) << p 
                 << std::fixed << std::setprecision(6) << std::setw(15) << res.speedup 
                 << std::setw(15) << res.efficiency << std::endl;
        
        resFile.flush();
        scalFile.flush();
    }
    
    resFile.close();
    scalFile.close();
}