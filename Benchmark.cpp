#include "Benchmark.h"
#include <omp.h>
#include <iostream>
#include <fstream>
#include <random>
#include <iomanip>
//
Benchmark::Benchmark(int reps, int s, double delta_t, unsigned int s_seed) 
    : repetitions(reps), steps(s), dt(delta_t), seed(s_seed) {}
std::string Benchmark::generateFileName(std::string prefix, int sync, int sched, int chunk) {
    return prefix + "_s" + std::to_string(sync) + 
           "_h" + std::to_string(sched) + 
           "_k" + std::to_string(chunk) + ".dat";
}

void Benchmark::setupRandomSystem(NBodySystem& system, int n) {
    std::mt19937 gen(seed); 
    std::uniform_real_distribution<> pos_dis(0, 1000);
    std::uniform_real_distribution<> mass_dis(1, 100);

    for (int i = 0; i < n; ++i) {
        system.addParticle(Particle(mass_dis(gen), pos_dis(gen), pos_dis(gen)));
    }
}

simulation_data Benchmark::runScalabilityTest(int max_threads, int num_particles, int sync_type) {
    std::ofstream resFile("benchmark_results.dat", std::ios::app);
    std::ofstream scalFile("scaling_analysis.dat", std::ios::app);
    
    // Anchos Columna
    const int W_S = 6;  // Sync
    const int W_H = 8;  // Threads
    const int W_D = 20; // Datos (Double)

    auto checkHeader = [&](std::ofstream& f, std::vector<std::string> cols) {
        f.seekp(0, std::ios::end);
        if (f.tellp() == 0) {
            f << "# ";
            f << std::left << std::setw(W_S-2) << cols[0]; // Ajuste por el "# "
            for(size_t i=1; i<cols.size(); ++i) f << std::setw(W_D) << cols[i];
            f << std::endl;
        }
    };

    // Encabezados con anchos rígidos
    checkHeader(resFile, {"Sync", "Threads", "T_Sim_Avg", "T_Sim_StdDev"});
    checkHeader(scalFile, {"Sync", "Threads", "Speedup", "Sigma_Sp", "Serial_F", "Theo_Sp", "Efficiency", "Sigma_Eff"});
    // --- 1. EJECUCIÓN SERIAL PURA (T1) ---
    std::vector<double> t1_times;
    std::cout << "Midiendo referencia Serial (T1)..." << std::endl;
    for (int r = 0; r < repetitions; ++r) {
        double start = omp_get_wtime();
        NBodySystem system(1.0, 10.0);
        setupRandomSystem(system, num_particles);
        NBodySimulator sim(&system, dt);
        sim.processBodies(steps); 
        t1_times.push_back(omp_get_wtime() - start);
    }

    // --- 2. BUCLE DE HILOS (Tp) ---
    for (int p = 1; p <= max_threads; p *= 2) {
        //tiempo serial
        std::vector<double> ts_times;
        //tiempo paralelo
        std::vector<double> tp_times;
        std::cout << "Corriendo con " << p << " hilos..." << std::endl;
        omp_set_num_threads(p);

        for (int r = 0; r < repetitions; ++r) {
            //Medicion de tiempos seriales
            double serialsum = 0.0;
            double serialStart = omp_get_wtime();

            NBodySystem system(1.0, 10.0);
            setupRandomSystem(system, num_particles);
            NBodySimulator sim(&system, dt);

            double parallelStart = omp_get_wtime();
            serialsum += omp_get_wtime() - serialStart; // Tiempo de setup (serial)

            simulation_data data = sim.processBodies(steps, sync_type);

            serialStart = omp_get_wtime();
            tp_times.push_back(omp_get_wtime() - parallelStart);
            serialsum += omp_get_wtime() - serialStart; // Tiempo de teardown (serial)
            ts_times.push_back(serialsum);

        }
        PerformanceResult res = MetricsCalculator::analyzePerformance(t1_times, tp_times, ts_times, p);


        resFile << std::left << std::setw(W_S)  << sync_type 
                << std::setw(W_H)  << p 
                << std::fixed << std::setprecision(10) 
                << std::setw(W_D)  << res.mean_time 
                << std::setw(W_D)  << res.std_dev << std::endl;

        // Escritura en scaling_analysis.dat alineada
        scalFile << std::left << std::setw(W_S)  << sync_type 
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
    
    resFile.close();
    scalFile.close();
    return simulation_data(); // Devolver los datos de la simulación
}
