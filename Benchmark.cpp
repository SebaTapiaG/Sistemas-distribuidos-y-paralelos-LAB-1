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
    std::string outName = "scalability_data_sync_" + std::to_string(sync_type) + ".dat";
    std::ofstream outFile(outName);
    
    outFile << "# Hilos | T_Sim_Avg | T_Sim_StdDev | Speedup_Exp" << std::endl;

    //Ejecucion Serial para obtener T1
    std::vector<double> t1_times;
    std::cout << "Midiendo version SERIAL (T1)..." << std::endl;

    for (int r = 0; r < repetitions; ++r) {
        NBodySystem system(1.0, 10.0);
        setupRandomSystem(system, num_particles);
        NBodySimulator sim(&system, dt);
        //Medicion de tiempo de ejecucion Serial (T1)
        double start_t1 = omp_get_wtime();
        sim.processBodies(steps); 
        t1_times.push_back(omp_get_wtime() - start_t1);
    }
    //Promedio de T1 para analisis
    double t1_mean = MetricsCalculator::calculateMean(t1_times);
    std::cout << "T1 Promedio: " << t1_mean << " segundos." << std::endl;

    //Ejecucion paralela, con multiplos potencia de 2 (Tp)
    for (int p = 1; p <= max_threads; p *= 2) {
        std::vector<double> tp_times;
        std::cout << "Analizando con " << p << " hilo(s) OpenMP..." << std::endl;
        
        omp_set_num_threads(p);

        for (int r = 0; r < repetitions; ++r) {
            //Setup
            NBodySystem system(1.0, 10.0);
            setupRandomSystem(system, num_particles);
            NBodySimulator sim(&system, dt);
            //Medicion de tiempos paralelos (Tp)
            double start_tp = omp_get_wtime();
            // Ejecuta la versión paralela con el método de sincronización elegido
            sim.processBodies(steps, sync_type);
            tp_times.push_back(omp_get_wtime() - start_tp);
        }

        double avg_tp = MetricsCalculator::calculateMean(tp_times);
        double dev_tp = MetricsCalculator::calculateStdDev(tp_times, avg_tp);
        double speedup = t1_mean / avg_tp;

        outFile << std::left << std::setw(6)  << p 
        << std::left << std::setw(15) << avg_tp 
        << std::left << std::setw(15) << dev_tp 
        << std::left << std::setw(15) << speedup << std::endl;

outFile.flush();
    }

    outFile.close();
    std::cout << "Resultados exportados a: " << outName << std::endl;
}