#include "Benchmark.h"
#include <omp.h>
#include <iostream>
#include <fstream>
#include <random>
#include <iomanip>
#include <numeric>
#include <chrono>

Benchmark::Benchmark(int reps, int s, double delta_t, unsigned int s_seed) 
    : repetitions(reps), steps(s), dt(delta_t), seed(s_seed) {
}

Benchmark::Benchmark(const NBodySystem& system, int num_steps, double delta_t, int reps)
    : base_system(system), steps(num_steps), dt(delta_t), repetitions(reps), seed(123) {}

// Ya no se usa
std::string Benchmark::generateFileName(std::string prefix, int /*sync_type*/, int /*task_type*/, int /*energy_method*/, int /*schedule_type*/, int /*chunk_size*/) {
    return prefix + ".dat";
}
// Generación de sistemas aleatorios para pruebas
// Este método se encarga de poblar el sistema con partículas de masa y posición aleatorias, utilizando la semilla proporcionada para garantizar reproducibilidad.
void Benchmark::setupRandomSystem(NBodySystem& system, int n) {
    std::mt19937 gen(seed); 
    std::uniform_real_distribution<> pos_dis(0, 1000);
    std::uniform_real_distribution<> mass_dis(1, 100);

    for (int i = 0; i < n; ++i) {
        system.addParticle(Particle(mass_dis(gen), pos_dis(gen), pos_dis(gen)));
    }
}
// Método principal para ejecutar la prueba de escalabilidad
// Este método ejecuta la simulación para un número variable de hilos, midiendo el tiempo de ejecución y calculando métricas de rendimiento como el speedup y la eficiencia. 
// También se encarga de escribir los resultados en archivos para su posterior análisis.
simulation_data Benchmark::runScalabilityTest(
    int max_threads, int num_particles, int task_type, 
    int sync_type, int energy_method, int schedule_type, 
    int chunk_size, double G, double epsilon, bool perform_diagnostics, int mode)
{
    if (perform_diagnostics) { // Fase de diagnóstico físico e integridad
    NBodySystem sys_diag(G, epsilon);
    setupRandomSystem(sys_diag, num_particles);
    NBodySimulator sim_diag(&sys_diag, dt);
    
    // Ejecutamos la simulación de prueba para obtener trayectoria, energía y cuerpos actualizados
    simulation_data diag_data = sim_diag.processBodies(steps, task_type, sync_type, 
                                                      energy_method, schedule_type, chunk_size);
    
    // 1. Análisis Físico de Energía y Momento
    PhysicalResult phys = MetricsCalculator::analyzePhysics(diag_data);
    
    // 2. Evolución del Centro de Masa (Inicial vs Final)
    CenterOfMass com_initial = MetricsCalculator::calculateCenterOfMass(diag_data.bodies.front());
    CenterOfMass com_final   = MetricsCalculator::calculateCenterOfMass(diag_data.bodies.back());
    
    double com_drift_x = std::abs(com_final.x - com_initial.x);
    double com_drift_y = std::abs(com_final.y - com_initial.y);

    // 3. Verificación de Integridad Paralela (Usando el último snapshot de diag_data)
    const std::vector<Particle>& final_bodies = diag_data.bodies.back();
    DiagnosticResult diag = MetricsCalculator::verifyConsistency(final_bodies);
    Particle serial_ref = final_bodies.back();

    // Guardado en archivo de diagnóstico
    std::ofstream diagFile("execution_integrity.dat", std::ios::app);
    diagFile << "=== DIAGNÓSTICO FÍSICO E INTEGRIDAD ===\n";
    diagFile << "Hilos: " << max_threads << " | N: " << num_particles << " | Pasos: " << steps << "\n";
    
    diagFile << std::fixed << std::setprecision(6);
    diagFile << "[FÍSICA] Energía Inicial: " << (diag_data.k.front() + diag_data.u.front()) 
             << " | Final: " << (diag_data.k.back() + diag_data.u.back()) 
             << " | Drift Relativo: " << phys.relative_error << "\n";
             
    diagFile << "[FÍSICA] Momento Mag. Inicial: " << phys.initial_momentum.magnitude 
             << " | Final: " << phys.final_momentum.magnitude << "\n";

    diagFile << "[CENTRO DE MASA] Inicial: (" << com_initial.x << ", " << com_initial.y << ")"
             << " -> Final: (" << com_final.x << ", " << com_final.y << ")"
             << " | Drift (dx, dy): (" << com_drift_x << ", " << com_drift_y << ")\n";

    diagFile << "[INTEGRIDAD] Checksum Masa -> Serial: " << serial_ref.getMass() 
             << " | Paralelo: " << diag.last_particle_state.getMass() << "\n";
    diagFile << "[INTEGRIDAD] Estado: " << (phys.is_valid && diag.consistency_pass ? "PASÓ" : "FALLÓ") << "\n";
    diagFile << "----------------------------------------------------\n\n";
    diagFile.close();
}
    // Usamos el rollback para centralizar todo en dos archivos
    std::string resFileName = generateFileName("bench_results", sync_type, task_type, energy_method, schedule_type, chunk_size);
    std::string scalFileName = generateFileName("scaling_analysis", sync_type, task_type, energy_method, schedule_type, chunk_size);

    std::ofstream compFile("amdahl_comparison.dat", std::ios::app);
     compFile << std::left << std::setw(10) << "Threads" 
                << std::setw(15) << "SerialFrac" 
                << std::setw(15) << "TheoricalF" 
                << std::setw(15) << "AbsDiff" << std::endl;
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

        PerformanceResult res = Benchmark::analyzePerformance(t1_times, tp_times, ts_times, p, mode);

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

                compFile << std::left << std::setw(10) << p 
                << std::setw(15) << res.serial_fraction
                << std::setw(15) << res.theorical_serial_fraction
                << std::setw(15) << std::abs(res.serial_fraction - res.theorical_serial_fraction) << std::endl;
        
        resFile.flush();
        scalFile.flush();
        compFile.flush();
        

    }
    compFile.close();
    scalFile.close();
    resFile.close();

    return final_data;
}
// Método que encapsula el análisis de rendimiento, permitiendo elegir entre el método tradicional o 
// el basado en firstprivate para el cálculo de desviaciones estándar.
PerformanceResult Benchmark::analyzePerformance(const std::vector<double>& t1_times, 
                                                      const std::vector<double>& tp_times, 
                                                      const std::vector<double>& ts_times,
                                                      int num_threads, int mode) {
    if (mode == 0) {
        return analyzePerformance(t1_times, tp_times, num_threads);
    } else {
        return analyzePerformanceFirstPrivate(t1_times, tp_times, ts_times, num_threads);
    }
}
//Método específico para el análisis de rendimiento de manera serial, calculando métricas como el speedup, la fracción serial y la eficiencia,
//  junto con sus respectivas desviaciones estándar para una interpretación estadística robusta.
PerformanceResult Benchmark::analyzePerformance(const std::vector<double>& t1_times, 
                                                      const std::vector<double>& tp_times,
                                                      int num_threads) {
    PerformanceResult res;
    
    double t1 = MetricsCalculator::calculateMean(t1_times);
    double sigma_t1 = MetricsCalculator::calculateStdDev(t1_times, t1);
    
    double tp = MetricsCalculator::calculateMean(tp_times);
    double sigma_tp = MetricsCalculator::calculateStdDev(tp_times, tp);

    //double ts = MetricsCalculator::calculateMean(ts_times);
    //double sigma_ts = MetricsCalculator::calculateStdDev(ts_times, ts);
    double tf= estimateSerialFractionByTime(t1, tp, num_threads);
    double theorical_speedup = calculateTheoricalSpeedup(tf, num_threads);
    
    res.mean_time = tp;
    res.std_dev = sigma_tp;
    res.speedup = t1 / tp;
    double f = estimateSerialFraction(res.speedup, num_threads);
    res.serial_fraction = f;
    res.theorical_serial_fraction = tf;
    res.theorical_speedup = theorical_speedup;
    // Propagación de error: sigma_Sp = Sp * sqrt((sigma_t1/t1)^2 + (sigma_tp/tp)^2)
    double rel_err_t1 = (t1 > 0) ? (sigma_t1 / t1) : 0;
    double rel_err_tp = (tp > 0) ? (sigma_tp / tp) : 0;
    //double rel_err_ts = (ts > 0) ? (sigma_ts / ts) : 0; no usada ya
    res.sigma_speedup = res.speedup * std::sqrt(rel_err_t1 * rel_err_t1 + rel_err_tp * rel_err_tp);
    res.efficiency = res.speedup / num_threads;
    res.sigma_efficiency = res.sigma_speedup / num_threads;
    res.sigma_serial_fraction = res.serial_fraction*(std::sqrt(rel_err_t1 * rel_err_t1 + rel_err_tp * rel_err_tp));
    res.sigma_theorical_speedup =  res.theorical_speedup*(std::sqrt(rel_err_t1 * rel_err_t1 + rel_err_tp * rel_err_tp));

    return res;
}
// Método específico para el análisis de rendimiento utilizando firstprivate, que paraleliza el cálculo de las 
// desviaciones estándar para mejorar la eficiencia en la interpretación estadística de los resultados.
PerformanceResult Benchmark::analyzePerformanceFirstPrivate(
    const std::vector<double>& t1_times, 
    const std::vector<double>& tp_times,
    const std::vector<double>& ts_times,
    int num_threads) 
{
    PerformanceResult res;
    double t1 = MetricsCalculator::calculateMean(t1_times);
    double tp = MetricsCalculator::calculateMean(tp_times);
    double ts = MetricsCalculator::calculateMean(ts_times);
    
    int n = tp_times.size();
    double t1_stdDev = 0.0;
    double tp_stdDev = 0.0;
    double ts_stdDev = 0.0;
    double speedup = (tp > 0.0) ? (t1 / tp) : 0.0;
    //se paraleliza el cálculo de las desviaciones estándar usando firstprivate 
    //para las medias y reduction para acumular las sumas de cuadrados de las diferencias
    #pragma omp parallel for firstprivate(t1, tp, ts) reduction(+:t1_stdDev, tp_stdDev, ts_stdDev)
    for (int i = 0; i < n; ++i) {
        t1_stdDev += (t1_times[i] - t1) * (t1_times[i] - t1);
        tp_stdDev += (tp_times[i] - tp) * (tp_times[i] - tp);
        ts_stdDev += (ts_times[i] - ts) * (ts_times[i] - ts);
    }
    double sigma_t1 = (n > 1) ? std::sqrt(t1_stdDev / (n - 1)) : 0.0;
    double sigma_tp = (n > 1) ? std::sqrt(tp_stdDev / (n - 1)) : 0.0;
    //double sigma_ts = (n > 1) ? std::sqrt(ts_stdDev / (n - 1)) : 0.0;
    double tf= estimateSerialFractionByTime(t1, tp, num_threads);
    double theorical_speedup = calculateTheoricalSpeedup(tf, num_threads);
    
    res.mean_time = tp;
    res.std_dev = sigma_tp;
    res.speedup = speedup;
    double f = estimateSerialFraction(res.speedup, num_threads);
    res.serial_fraction = f;
    res.theorical_serial_fraction = tf;
    res.theorical_speedup = theorical_speedup;
    // Propagación de error: sigma_Sp = Sp * sqrt((sigma_t1/t1)^2 + (sigma_tp/tp)^2)
    // Esto es vital para las barras de error en tus gráficos
    double rel_err_t1 = (t1 > 0) ? (sigma_t1 / t1) : 0;
    double rel_err_tp = (tp > 0) ? (sigma_tp / tp) : 0;
    //double rel_err_ts = (ts > 0) ? (sigma_ts / ts) : 0; no e usa ya
    res.sigma_speedup = res.speedup * std::sqrt(rel_err_t1 * rel_err_t1 + rel_err_tp * rel_err_tp);
    res.efficiency = res.speedup / num_threads;
    res.sigma_efficiency = res.sigma_speedup / num_threads;
    res.sigma_serial_fraction = res.serial_fraction*(std::sqrt(rel_err_t1 * rel_err_t1 + rel_err_tp * rel_err_tp));
    res.sigma_theorical_speedup =  res.theorical_speedup*(std::sqrt(rel_err_t1 * rel_err_t1 + rel_err_tp * rel_err_tp));
    // ... (puedes incluir aquí el resto de tus cálculos de sigma)
    return res;
}
//Metodo de calculo de la fracción serial a partir del speedup medido, basado en la Ley de Amdahl.
double Benchmark::estimateSerialFraction(double speedup, int p) {
    if (p <= 1){
        return 1.0;
    }
    // Basado en Ley de Amdahl despejada para 'f'
    double f = ((1.0 / speedup) - (1.0 / (double)p)) / (1.0 - (1.0 / (double)p));
    return (f < 0) ? 0 : f; // Ajuste por ruido experimental
}
// Metodo de estimación de la fracción serial a partir de los tiempos medidos, 
// En Benchmark.cpp:
double Benchmark::estimateSerialFractionByTime(double t1, double tp, int p) {
    // Si la simulación corre en 1 solo hilo o los tiempos son inválidos, f = 1.0 (100% serial)
    if (t1 <= 0.0 || tp <= 0.0 || p <= 1) {
        return 1.0;
    }

    // 1. Despeje de la fracción paralelizable Tp según la deducción del ayudante
    double p_double = static_cast<double>(p);
    double tp_paralelizable = (p_double / (p_double - 1.0)) * (t1 - tp);

    // 2. Tiempo de la fracción serial Ts
    double ts_serial = t1 - tp_paralelizable;

    // 3. Fracción serial normalizada f
    double f = ts_serial / t1;

    // Control de límites por posible ruido experimental (mediciones de tiempo con pequeñas variaciones)
    if (f < 0.0) return 0.0;
    if (f > 1.0) return 1.0;

    return f;
}
// Metodo de calculo del speedup teorico segun la ley de amdahl, utilizando la fraccion serial estimada a partir de los tiempos medidos.
double Benchmark::calculateTheoricalSpeedup(double f, int p) {
    if (p <= 1){
        return 1.0;
    }
    return 1.0 / (f + ((1.0 - f) / (double)p));
}

//METODS DE BENCHMARKING PARA CUDA
// Auxiliar para calculo de media y desviación estándar de un vector de tiempos en milisegundos
MeasurementResult Benchmark::calculateStats(const std::vector<double>& times_ms) {
    if (times_ms.empty()) return {0.0, 0.0};
    double sum = std::accumulate(times_ms.begin(), times_ms.end(), 0.0);
    double mean = sum / times_ms.size();
    double sq_sum = 0.0;
    for (double t : times_ms) sq_sum += (t - mean) * (t - mean);
    double stddev = (times_ms.size() > 1) ? std::sqrt(sq_sum / (times_ms.size() - 1)) : 0.0;
    return {mean, stddev};
}
bool Benchmark::verifyCpuGpuCorrectness(NBodySimulator& sim_cpu, NBodySimulator& sim_gpu, 
                                        int variant, int block_size, double tolerance) {
    sim_cpu.runCpuSerial(1, dt);
    sim_gpu.stepGpuEndToEnd(variant, block_size);

    const auto& bodies_cpu = sim_cpu.getSystem().getBodies();
    const auto& bodies_gpu = sim_gpu.getSystem().getBodies();

    if (bodies_cpu.size() != bodies_gpu.size()) return false;

    double max_diff = 0.0;
    for (size_t i = 0; i < bodies_cpu.size(); ++i) {
        double diff_ax = std::abs(bodies_cpu[i].getAx() - bodies_gpu[i].getAx());
        double diff_ay = std::abs(bodies_cpu[i].getAy() - bodies_gpu[i].getAy());

        if (diff_ax > max_diff) max_diff = diff_ax;
        if (diff_ay > max_diff) max_diff = diff_ay;

        if (diff_ax > tolerance || diff_ay > tolerance) {
            std::cerr << "[ERROR] Divergencia excesiva en cuerpo " << i 
                      << " | Max Diff: " << max_diff << " (Tolerancia: " << tolerance << ")\n";
            return false;
        }
    }

    std::cout << "[VERIFICADO] Pruebas CPU vs GPU DENTRO de tolerancia (" 
              << tolerance << "). Dif Max: " << max_diff << std::endl;
    return true;
}

MeasurementResult Benchmark::benchmarkCpuSerial(int runs) {
    std::vector<double> times_ms;
    times_ms.reserve(runs);

    for (int r = 0; r < runs; ++r) {
        NBodySystem sys_copy = base_system;
        NBodySimulator sim_cpu(sys_copy);

        auto start = std::chrono::steady_clock::now();
        sim_cpu.runCpuSerial(steps, dt);
        auto end = std::chrono::steady_clock::now();

        times_ms.push_back(std::chrono::duration<double, std::milli>(end - start).count());
    }

    return calculateStats(times_ms);
}

MeasurementResult Benchmark::benchmarkKernelOnly(NBodySimulator& simulator, int variant, int block_size, int runs) {
    std::vector<double> times_ms;
    times_ms.reserve(runs);

    simulator.prepareGpu(); 

    for (int r = 0; r < runs; ++r) {
        cudaDeviceSynchronize();
        auto start = std::chrono::steady_clock::now();

        simulator.stepGpuKernelOnly(variant, block_size);

        cudaDeviceSynchronize();
        auto end = std::chrono::steady_clock::now();

        times_ms.push_back(std::chrono::duration<double, std::milli>(end - start).count());
    }

    simulator.finishGpu();
    return calculateStats(times_ms);
}

MeasurementResult Benchmark::benchmarkEndToEnd(NBodySimulator& simulator, int variant, int block_size, int runs) {
    std::vector<double> times_ms;
    times_ms.reserve(runs);

    for (int r = 0; r < runs; ++r) {
        cudaDeviceSynchronize();
        auto start = std::chrono::steady_clock::now();

        simulator.stepGpuEndToEnd(variant, block_size);

        cudaDeviceSynchronize();
        auto end = std::chrono::steady_clock::now();

        times_ms.push_back(std::chrono::duration<double, std::milli>(end - start).count());
    }

    return calculateStats(times_ms);
}

CpuGpuComparison Benchmark::compareCpuGpuKernelOnly(int n_bodies, int variant, int block_size, int runs) {
    MeasurementResult cpu_res = benchmarkCpuSerial(runs);

    NBodySystem sys_gpu = base_system;
    NBodySimulator sim_gpu(sys_gpu);
    MeasurementResult kernel_res = benchmarkKernelOnly(sim_gpu, variant, block_size, runs);

    double speedup = (kernel_res.mean_ms > 0.0) ? (cpu_res.mean_ms / kernel_res.mean_ms) : 0.0;

    return { n_bodies, variant, block_size, cpu_res, kernel_res, {0.0, 0.0}, speedup, 0.0, 0.0 };
}

CpuGpuComparison Benchmark::compareCpuGpuEndToEnd(int n_bodies, int variant, int block_size, int runs) {
    MeasurementResult cpu_res = benchmarkCpuSerial(runs);

    NBodySystem sys_gpu = base_system;
    NBodySimulator sim_gpu(sys_gpu);
    MeasurementResult e2e_res = benchmarkEndToEnd(sim_gpu, variant, block_size, runs);

    double speedup = (e2e_res.mean_ms > 0.0) ? (cpu_res.mean_ms / e2e_res.mean_ms) : 0.0;

    return { n_bodies, variant, block_size, cpu_res, {0.0, 0.0}, e2e_res, 0.0, speedup, 0.0 };
}

CpuGpuComparison Benchmark::compareCpuGpu(int n_bodies, int variant, int block_size, int runs) {
    MeasurementResult cpu_res = benchmarkCpuSerial(runs);

    NBodySystem sys_gpu1 = base_system;
    NBodySimulator sim_gpu1(sys_gpu1);
    MeasurementResult kernel_res = benchmarkKernelOnly(sim_gpu1, variant, block_size, runs);

    NBodySystem sys_gpu2 = base_system;
    NBodySimulator sim_gpu2(sys_gpu2);
    MeasurementResult e2e_res = benchmarkEndToEnd(sim_gpu2, variant, block_size, runs);

    double speedup_kernel = (kernel_res.mean_ms > 0.0) ? (cpu_res.mean_ms / kernel_res.mean_ms) : 0.0;
    double speedup_e2e    = (e2e_res.mean_ms > 0.0)    ? (cpu_res.mean_ms / e2e_res.mean_ms)    : 0.0;

    double overhead_ms = e2e_res.mean_ms - kernel_res.mean_ms;
    double amdahl_f = (cpu_res.mean_ms > 0.0) ? (overhead_ms / cpu_res.mean_ms) : 0.0;
    if (amdahl_f < 0.0) amdahl_f = 0.0;

    return {
        n_bodies, variant, block_size,
        cpu_res, kernel_res, e2e_res,
        speedup_kernel, speedup_e2e, amdahl_f
    };
}

std::vector<CpuGpuComparison> Benchmark::runSuite40(const std::vector<int>& n_particles_list, 
                                                     int mode,
                                                     const std::vector<int>& block_sizes, 
                                                     int runs) {
    std::vector<CpuGpuComparison> results;
    results.reserve(n_particles_list.size() * 2 * block_sizes.size());

    for (int N : n_particles_list) {
        // Re-generar el sistema base con N partículas para esta iteración
        base_system = NBodySystem(1.0, 1e-3); // Reconstruir con G y epsilon por defecto
        setupRandomSystem(base_system, N);

        for (int variant : {0, 1}) {
            for (int block : block_sizes) {
                CpuGpuComparison comp;

                if (mode == 0) {
                    std::cout << "[SUITE KERNEL 40] N: " << N << " | Var: " << variant << " | Block: " << block << "...\n";
                    comp = compareCpuGpuKernelOnly(N, variant, block, runs);
                } 
                else if (mode == 1) {
                    std::cout << "[SUITE E2E 40] N: " << N << " | Var: " << variant << " | Block: " << block << "...\n";
                    comp = compareCpuGpuEndToEnd(N, variant, block, runs);
                } 
                else {
                    std::cout << "[SUITE COMPLETA] N: " << N << " | Var: " << variant << " | Block: " << block << "...\n";
                    comp = compareCpuGpu(N, variant, block, runs);
                }

                results.push_back(comp);
            }
        }
    }

    return results;
}

void Benchmark::exportComparisonToCSV(const std::string& filename, const std::vector<CpuGpuComparison>& results) {
    std::ofstream file(filename);
    file << "N,Variant,BlockSize,CPU_Mean_ms,CPU_StdDev,Kernel_Mean_ms,Kernel_StdDev,E2E_Mean_ms,E2E_StdDev,Speedup_Kernel,Speedup_E2E,Amdahl_f\n";

    for (const auto& r : results) {
        file << r.num_particles << ","
             << r.variant << ","
             << r.block_size << ","
             << std::fixed << std::setprecision(4)
             << r.cpu_serial.mean_ms << "," << r.cpu_serial.stddev_ms << ","
             << r.gpu_kernel.mean_ms << "," << r.gpu_kernel.stddev_ms << ","
             << r.gpu_end_to_end.mean_ms << "," << r.gpu_end_to_end.stddev_ms << ","
             << r.speedup_kernel << ","
             << r.speedup_e2e << ","
             << r.amdahl_f << "\n";
    }
    file.close();
    std::cout << "Resultados guardados exitosamente en: " << filename << std::endl;
}
// ── 1. Generación de benchmark_results.dat ──────────────────────────────────
void Benchmark::exportBenchmarkResultsDAT(const std::string& filename, const std::vector<CpuGpuComparison>& results) {
    std::ofstream file(filename);
    
    // Encabezados tabulares
    file << std::left 
         << std::setw(10) << "N" 
         << std::setw(10) << "Variant" 
         << std::setw(12) << "BlockSize" 
         << std::setw(16) << "CPU_Mean_ms" 
         << std::setw(16) << "CPU_StdDev" 
         << std::setw(16) << "Kernel_Mean_ms" 
         << std::setw(16) << "Kernel_StdDev" 
         << std::setw(16) << "E2E_Mean_ms" 
         << std::setw(16) << "E2E_StdDev" << "\n";

    file << std::fixed << std::setprecision(6);
    for (const auto& r : results) {
        file << std::left 
             << std::setw(10) << r.num_particles
             << std::setw(10) << r.variant
             << std::setw(12) << r.block_size
             << std::setw(16) << r.cpu_serial.mean_ms
             << std::setw(16) << r.cpu_serial.stddev_ms
             << std::setw(16) << r.gpu_kernel.mean_ms
             << std::setw(16) << r.gpu_kernel.stddev_ms
             << std::setw(16) << r.gpu_end_to_end.mean_ms
             << std::setw(16) << r.gpu_end_to_end.stddev_ms << "\n";
    }
    file.close();
    std::cout << "[VISUALIZER] Generado: " << filename << std::endl;
}

// ── 2. Generación de scaling_analysis.dat ───────────────────────────────────
void Benchmark::exportScalingAnalysisDAT(const std::string& filename, const std::vector<CpuGpuComparison>& results) {
    std::ofstream file(filename);
    
    file << std::left 
         << std::setw(10) << "N" 
         << std::setw(10) << "Variant" 
         << std::setw(12) << "BlockSize" 
         << std::setw(16) << "Speedup_Kernel" 
         << std::setw(16) << "Speedup_E2E" 
         << std::setw(16) << "SerialFrac_f" << "\n";

    file << std::fixed << std::setprecision(6);
    for (const auto& r : results) {
        file << std::left 
             << std::setw(10) << r.num_particles
             << std::setw(10) << r.variant
             << std::setw(12) << r.block_size
             << std::setw(16) << r.speedup_kernel
             << std::setw(16) << r.speedup_e2e
             << std::setw(16) << r.amdahl_f << "\n";
    }
    file.close();
    std::cout << "[VISUALIZER] Generado: " << filename << std::endl;
}

// ── 3. Generación de blockdim_study.dat ─────────────────────────────────────
void Benchmark::exportBlockDimStudyDAT(const std::string& filename, const std::vector<CpuGpuComparison>& results) {
    std::ofstream file(filename);
    
    // Vista especializada para analizar el comportamiento al variar el tamaño de bloque
    file << std::left 
         << std::setw(12) << "BlockSize" 
         << std::setw(10) << "N" 
         << std::setw(10) << "Variant" 
         << std::setw(16) << "Kernel_Mean_ms" 
         << std::setw(16) << "E2E_Mean_ms" 
         << std::setw(16) << "Speedup_Kernel" << "\n";

    file << std::fixed << std::setprecision(6);
    for (const auto& r : results) {
        file << std::left 
             << std::setw(12) << r.block_size
             << std::setw(10) << r.num_particles
             << std::setw(10) << r.variant
             << std::setw(16) << r.gpu_kernel.mean_ms
             << std::setw(16) << r.gpu_end_to_end.mean_ms
             << std::setw(16) << r.speedup_kernel << "\n";
    }
    file.close();
    std::cout << "[VISUALIZER] Generado: " << filename << std::endl;
}

// ── Integrador: Ejecuta y exporta las 3 salidas automáticamente ──────────────
void Benchmark::runAndExportAllDAT(const std::vector<int>& n_particles_list, 
                                   const std::vector<int>& block_sizes, 
                                   int runs) {
    std::cout << "\n=== EJECUTANDO BENCHMARK COMPLETO PARA GPU VISUALIZER ===\n";
    
    // Modo 2 ejecuta comparaciones completas (CPU, Kernel y E2E)
    auto results = runSuite40(n_particles_list, 2, block_sizes, runs);

    // Genera los 3 archivos .dat en el directorio raíz de ejecución
    exportBenchmarkResultsDAT("benchmark_results.dat", results);
    exportScalingAnalysisDAT("scaling_analysis.dat", results);
    exportBlockDimStudyDAT("blockdim_study.dat", results);

    std::cout << "=== TODOS LOS ARCHIVOS .DAT FUERON GENERADOS CON ÉXITO ===\n\n";
}