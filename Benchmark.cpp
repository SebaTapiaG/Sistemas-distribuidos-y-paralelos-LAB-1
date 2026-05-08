#include "Benchmark.h"
#include <omp.h>
#include <iostream>
#include <fstream>
#include <random>
#include <iomanip>

Benchmark::Benchmark(int reps, int s, double delta_t, unsigned int s_seed) 
    : repetitions(reps), steps(s), dt(delta_t), seed(s_seed) {
}

// Omitimos los nombres de las variables que no se usan para evitar los warnings del compilador
std::string Benchmark::generateFileName(std::string prefix, int /*sync_type*/, int /*task_type*/, int /*energy_method*/, int /*schedule_type*/, int /*chunk_size*/) {
    return prefix + ".dat";
}

void Benchmark::setupRandomSystem(NBodySystem& system, int n) {
    std::mt19937 gen(seed); 
    std::uniform_real_distribution<> pos_dis(0, 1000);
    std::uniform_real_distribution<> mass_dis(1, 100);

    for (int i = 0; i < n; ++i) {
        system.addParticle(Particle(mass_dis(gen), pos_dis(gen), pos_dis(gen)));
    }
}

simulation_data Benchmark::runScalabilityTest(
    int max_threads, int num_particles, int task_type, 
    int sync_type, int energy_method, int schedule_type, 
    int chunk_size, double G, double epsilon)
{
    // Usamos el rollback para centralizar todo en dos archivos
    std::string resFileName = generateFileName("bench_results", sync_type, task_type, energy_method, schedule_type, chunk_size);
    std::string scalFileName = generateFileName("scaling_analysis", sync_type, task_type, energy_method, schedule_type, chunk_size);

    std::ofstream resFile(resFileName, std::ios::app);
    std::ofstream scalFile(scalFileName, std::ios::app);

    const int W_S = 10; // Para Sync Type y Sched
    const int W_H = 8;  // Para Threads
    const int W_D = 15; // Para Datos (Double)
    
    // Solo escribimos encabezados para referencia
    resFile << std::left << std::setw(W_S) << "SyncType" 
        << std::setw(W_H) << "Threads" 
        << std::setw(W_D) << "MeanTime" 
        << std::setw(W_D) << "StdDev" << std::endl;
        
    scalFile << std::left << std::setw(W_S) << "SyncType"
         << std::setw(W_S) << "Sched"    
         << std::setw(W_S) << "Chunk"
         << std::setw(W_H) << "Threads" 
         << std::setw(W_D) << "Speedup" 
         << std::setw(W_D) << "SigmaSpeedup"
         << std::setw(W_D) << "SerialFrac" 
         << std::setw(W_D) << "SigmaSerialFrac"
         << std::setw(W_D) << "TheoSpeedup"
         << std::setw(W_D) << "SigmaTheoSpeedup"
         << std::setw(W_D) << "Efficiency"
         << std::setw(W_D) << "SigmaEfficiency" << std::endl;

    simulation_data final_data;
    std::vector<double> t1_times;
    
    // Ejecución serial T1
    for (int r = 0; r < repetitions; ++r) {
        NBodySystem system(G, epsilon);
        setupRandomSystem(system, num_particles);
        NBodySimulator simulator(&system, dt);
        
        // El cronómetro DEBE ir después del setup para no contaminar el tiempo serial
        double start = omp_get_wtime();
        simulator.processBodies(steps); 
        t1_times.push_back(omp_get_wtime() - start);
    }

    // Bucle paralelo Tp 
    for (int p = 1; p <= max_threads; p *= 2) {
        std::vector<double> ts_times;
        std::vector<double> tp_times;
        omp_set_num_threads(p);

        for (int r = 0; r < repetitions; ++r) {
            double serialsum = 0.0;
            double serialStart = omp_get_wtime();
            NBodySystem system(G, epsilon);
            setupRandomSystem(system, num_particles);
            NBodySimulator simulator(&system, dt);
            serialsum += omp_get_wtime() - serialStart;
            
            double parallelStart = omp_get_wtime();
            final_data = simulator.processBodies(steps, task_type, sync_type, 
                         energy_method, schedule_type, chunk_size);
            tp_times.push_back(omp_get_wtime() - parallelStart);
            ts_times.push_back(serialsum);
        }

        PerformanceResult res = MetricsCalculator::analyzePerformance(t1_times, tp_times, ts_times, p);

        resFile << std::left << std::setw(W_S)  << sync_type 
                << std::setw(W_H)  << p 
                << std::fixed << std::setprecision(10) 
                << std::setw(W_D)  << res.mean_time 
                << std::setw(W_D)  << res.std_dev << std::endl;

        // Escritura en scaling_analysis.dat con las 12 columnas exactas
        scalFile << std::left << std::setw(W_S)  << sync_type 
                 << std::setw(W_S)  << schedule_type  // Agregado
                 << std::setw(W_S)  << chunk_size     // Agregado
                 << std::setw(W_H)  << p 
                 << std::fixed << std::setprecision(10)
                 << std::setw(W_D)  << res.speedup
                 << std::setw(W_D)  << res.sigma_speedup
                 << std::setw(W_D)  << res.serial_fraction
                 << std::setw(W_D)  << res.sigma_serial_fraction
                 << std::setw(W_D)  << res.theorical_speedup 
                 << std::setw(W_D)  << res.sigma_theorical_speedup
                 << std::setw(W_D)  << res.efficiency
                 << std::setw(W_D)  << res.sigma_efficiency << std::endl;
        
        resFile.flush();
        scalFile.flush();
    }
    scalFile.close();
    resFile.close();

    return final_data;
}