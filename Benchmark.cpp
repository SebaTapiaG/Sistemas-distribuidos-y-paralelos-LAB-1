#include "Benchmark.h"
#include <omp.h>
#include <iostream>
#include <fstream>
#include <random>
#include <iomanip>
#include <numeric>
#include <chrono>

// En Benchmark.cpp

// Constructor 1
Benchmark::Benchmark(int reps, int s, double delta_t, unsigned int s_seed) 
    : base_system(1.0, 0.01), repetitions(reps), steps(s), dt(delta_t), seed(s_seed) {
}

// Constructor 2
Benchmark::Benchmark(const NBodySystem& system, int num_steps, double delta_t, int reps)
    : base_system(system), repetitions(reps), steps(num_steps), dt(delta_t), seed(123) {
}


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

// ── Método Auxiliar de Estadística ──────────────────────────────────────────
MeasurementResult Benchmark::calculateStats(const std::vector<double>& times_ms) {
    MeasurementResult result;
    if (times_ms.empty()) {
        result.mean_ms = 0.0;
        result.stddev_ms = 0.0;
        return result;
    }

    double sum = 0.0;
    for (double t : times_ms) {
        sum += t;
    }
    result.mean_ms = sum / static_cast<double>(times_ms.size());

    double sq_diff_sum = 0.0;
    for (double t : times_ms) {
        double diff = t - result.mean_ms;
        sq_diff_sum += diff * diff;
    }

    if (times_ms.size() > 1) {
        result.stddev_ms = std::sqrt(sq_diff_sum / static_cast<double>(times_ms.size() - 1));
    } else {
        result.stddev_ms = 0.0;
    }

    return result;
}

// ── 1. Benchmark CPU Serial ──────────────────────────────────────────────────
MeasurementResult Benchmark::benchmarkCpuSerial(int runs) {
    std::vector<double> times_ms;
    times_ms.reserve(runs);

    for (int r = 0; r < runs; ++r) {
        // Recreamos el sistema para garantizar las mismas condiciones iniciales
        NBodySystem system = base_system;
        NBodySimulator simulator(&system, dt);

        auto start = std::chrono::high_resolution_clock::now();
        
        // Simulación en CPU sin transferencias ni GPU
        simulator.processBodies(steps);

        auto end = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
        times_ms.push_back(elapsed_ms);
    }

    return calculateStats(times_ms);
}

// ── 2. Benchmark GPU Kernel Only ─────────────────────────────────────────────
MeasurementResult Benchmark::benchmarkKernelOnly(
    NBodySimulator& simulator, 
    int variant, 
    int energy_method, 
    int block_size,
    double* d_u_ptr, 
    double* d_k_ptr, 
    int runs) 
{
    std::vector<double> times_ms;
    times_ms.reserve(runs);

    // Warm-up (corrida de calentamiento fuera de tiempo)
    simulator.stepEulerGpu(variant, energy_method, block_size, d_u_ptr, d_k_ptr);
    CUDA_CHECK(cudaDeviceSynchronize());

    for (int r = 0; r < runs; ++r) {
        // Restablecemos las partículas del simulador al estado base inicial en la GPU
        // (Asegura mismas condiciones en cada 'run' sin medir transferencias dentro del cronómetro)
        simulator.resetGpuStateFromBase(); // O recargar los buffers D2D / H2D fuera del cronómetro

        // INICIO CRONÓMETRO
        auto start = std::chrono::high_resolution_clock::now();

        // Ejecución de los N pasos temporales solo en GPU
        for (int s = 0; s < steps; ++s) {
            simulator.stepEulerGpu(variant, energy_method, block_size, d_u_ptr, d_k_ptr);
        }

        // Sincronización explícita indispensable para detener el tiempo real
        CUDA_CHECK(cudaDeviceSynchronize());

        // FIN CRONÓMETRO
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
        times_ms.push_back(elapsed_ms);
    }

    return calculateStats(times_ms);
}

// ── 3. Benchmark GPU End-to-End ──────────────────────────────────────────────
MeasurementResult Benchmark::benchmarkEndToEnd(
    int variant, 
    int energy_method, 
    int block_size, 
    int runs) 
{
    std::vector<double> times_ms;
    times_ms.reserve(runs);

    for (int r = 0; r < runs; ++r) {
        NBodySystem system = base_system;
        NBodySimulator sim_local(&system, dt);
        // INICIO: Medición completa (Instanciación H2D + Cómputo + D2H)
        auto start = std::chrono::high_resolution_clock::now();
        
        sim_local.processBodiesGpu(steps, variant, energy_method, block_size, false);
        CUDA_CHECK(cudaDeviceSynchronize());
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
        times_ms.push_back(elapsed_ms);
    }

    return calculateStats(times_ms);
}
// ─────────────────────────────────────────────────────────────────────────────
// 1. MÉTODOS DE ESCRITURA DIFERENCIADA
// ─────────────────────────────────────────────────────────────────────────────

// A. ARCHIVO MAESTRO: benchmark_results.dat
void Benchmark::writeFullResultsHeader(const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    if (!file.is_open()) return;

    file << std::left 
         << std::setw(8)  << "N"
         << std::setw(8)  << "Variant"
         << std::setw(10) << "BlockSize"
         << std::setw(16) << "CpuSerial_ms"
         << std::setw(16) << "CpuStdDev_ms"
         << std::setw(16) << "GpuKernel_ms"
         << std::setw(16) << "KernelStdDev_ms"
         << std::setw(16) << "GpuE2E_ms"
         << std::setw(16) << "E2EStdDev_ms"
         << std::setw(16) << "SpeedupKernel"
         << std::setw(16) << "SpeedupE2E"
         << std::setw(16) << "Amdahl_f"
         << std::setw(10) << "Accuracy"
         << std::endl;
}

void Benchmark::saveFullResultRow(const std::string& filename, const CpuGpuComparison& res) {
    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) return;

    file << std::left 
         << std::setw(8)  << res.num_particles
         << std::setw(8)  << res.variant
         << std::setw(10) << res.block_size
         << std::fixed    << std::setprecision(6)
         << std::setw(16) << res.cpu_serial.mean_ms
         << std::setw(16) << res.cpu_serial.stddev_ms
         << std::setw(16) << res.gpu_kernel.mean_ms
         << std::setw(16) << res.gpu_kernel.stddev_ms
         << std::setw(16) << res.gpu_end_to_end.mean_ms
         << std::setw(16) << res.gpu_end_to_end.stddev_ms
         << std::setw(16) << res.speedup_kernel
         << std::setw(16) << res.speedup_e2e
         << std::setw(16) << res.amdahl_f
         << std::setw(10) << (res.accuracy_pass ? "PASS" : "FAIL")
         << std::endl;
}

// B. ESTUDIO DE ESCALABILIDAD: scaling_analysis.dat (incluye Overhead explícito)
void Benchmark::writeScalingHeader(const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    if (!file.is_open()) return;

    file << std::left 
         << std::setw(8)  << "N"
         << std::setw(8)  << "Variant"
         << std::setw(10) << "BlockSize"
         << std::setw(16) << "CpuTime_ms"
         << std::setw(16) << "GpuKernel_ms"
         << std::setw(16) << "GpuE2E_ms"
         << std::setw(16) << "Overhead_ms"
         << std::setw(16) << "Serial_frac_E2E"
         << std::setw(16) << "SpeedupKernel"
        << std::setw(16) << "TheoLimit_Kernel"
         << std::setw(16) << "SpeedupE2E"
         << std::setw(16) << "TheoLimit_E2E"
         << std::endl;
}

void Benchmark::saveScalingRow(const std::string& filename, const CpuGpuComparison& res) {
    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) return;

    double cpu_ms    = res.cpu_serial.mean_ms;
    double kernel_ms = res.gpu_kernel.mean_ms;
    double e2e_ms    = std::max(res.gpu_end_to_end.mean_ms, kernel_ms);
    double overhead_ms = e2e_ms - kernel_ms;

    // 1. Fracción serial del sistema End-to-End (respecto a la CPU)
    double f_serial_e2e = (cpu_ms > 0.0) ? (overhead_ms / cpu_ms) : 0.0;

    // 2. Speedups y Límites Teóricos
    double sp_kernel = (kernel_ms > 0.0) ? (cpu_ms / kernel_ms) : 1.0;
    double sp_e2e    = (e2e_ms > 0.0) ? (cpu_ms / e2e_ms) : 1.0;

    double theo_limit_kernel = sp_kernel; // Techo puro del Kernel
    double theo_limit_e2e    = (f_serial_e2e > 0.0) 
        ? (1.0 / (f_serial_e2e + ((1.0 - f_serial_e2e) / sp_kernel))) 
        : sp_kernel;

    file << std::left 
         << std::setw(8)  << res.num_particles
         << std::setw(8)  << res.variant
         << std::setw(10) << res.block_size
         << std::fixed    << std::setprecision(6)
         << std::setw(16) << cpu_ms
         << std::setw(16) << kernel_ms
         << std::setw(16) << e2e_ms
         << std::setw(16) << overhead_ms
         << std::setw(16) << f_serial_e2e       // <-- FRACCIÓN SERIAL AGREGADA
         << std::setw(16) << sp_kernel
         << std::setw(16) << theo_limit_kernel
         << std::setw(16) << sp_e2e
         << std::setw(16) << theo_limit_e2e
         << std::endl;
}
// C. DIMENSIÓN DE BLOQUE: blockdim_study.dat
// C. DIMENSIÓN DE BLOQUE: blockdim_study.dat (VERSIÓN ACTUALIZADA Y COMPLETA)
void Benchmark::writeBlockDimHeader(const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    if (!file.is_open()) return;

    file << std::left 
         << std::setw(10) << "BlockSize"
         << std::setw(8)  << "Variant"
         << std::setw(8)  << "N"
         << std::setw(16) << "GpuKernel_ms"
         << std::setw(16) << "KernelStdDev"
         << std::setw(16) << "GpuE2E_ms"
         << std::setw(16) << "E2EStdDev"
         << std::setw(16) << "SpeedupKernel"
         << std::setw(16) << "SpeedupE2E"
         << std::setw(16) << "Amdahl_f"
         << std::setw(10) << "Accuracy"
         << std::endl;
}

void Benchmark::saveBlockDimRow(const std::string& filename, const CpuGpuComparison& res) {
    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) return;

    file << std::left 
         << std::setw(10) << res.block_size
         << std::setw(8)  << res.variant
         << std::setw(8)  << res.num_particles
         << std::fixed    << std::setprecision(6)
         << std::setw(16) << res.gpu_kernel.mean_ms
         << std::setw(16) << res.gpu_kernel.stddev_ms
         << std::setw(16) << res.gpu_end_to_end.mean_ms
         << std::setw(16) << res.gpu_end_to_end.stddev_ms
         << std::setw(16) << res.speedup_kernel
         << std::setw(16) << res.speedup_e2e         
         << std::setw(16) << res.amdahl_f             
         << std::setw(10) << (res.accuracy_pass ? "PASS" : "FAIL") 
         << std::endl;
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. ORQUESTADOR DE COMPARACIÓN CPU vs GPU (Con Tolerancia Integrada)
// ─────────────────────────────────────────────────────────────────────────────

CpuGpuComparison Benchmark::runGpuComparisonTest(
    const std::string& outputFile,
    int num_particles,
    int variant,
    int block_size,
    int energy_method,
    int runs,
    double rtol,
    double atol) 
{
    CpuGpuComparison result;
    result.num_particles = num_particles;
    result.variant = variant;
    result.block_size = block_size;

    // A. VERIFICACIÓN DE TOLERANCIA FÍSICA Y MATEMÁTICA
    NBodySystem sys_check_cpu = base_system;
    NBodySystem sys_check_gpu = base_system;

    NBodySimulator sim_check_cpu(&sys_check_cpu, dt);
    NBodySimulator sim_check_gpu(&sys_check_gpu, dt);

    // 1 paso en CPU y 1 paso en GPU con descarga de datos (record_frames = true)
    sim_check_cpu.processBodies(1);
    sim_check_gpu.processBodiesGpu(1, variant, energy_method, block_size, true);

    const auto& cpu_bodies = sys_check_cpu.getBodies(); // getBodies() en lugar de getParticles()
    const auto& gpu_bodies = sys_check_gpu.getBodies();

    // Extraer aceleraciones reales de las partículas
    std::vector<double> cpu_ax, cpu_ay, gpu_ax, gpu_ay;
    cpu_ax.reserve(num_particles); cpu_ay.reserve(num_particles);
    gpu_ax.reserve(num_particles); gpu_ay.reserve(num_particles);

    for (int i = 0; i < num_particles; ++i) {
        cpu_ax.push_back(cpu_bodies[i].getAx());
        cpu_ay.push_back(cpu_bodies[i].getAy());

        gpu_ax.push_back(gpu_bodies[i].getAx());
        gpu_ay.push_back(gpu_bodies[i].getAy());
    }

    AccuracyResult accuracy = MetricsCalculator::compareCpuGpuAccuracy(
        cpu_bodies, gpu_bodies,
        gpu_ax, gpu_ay, cpu_ax, cpu_ay,
        rtol, atol
    );

    result.accuracy_pass = accuracy.passed;
    result.max_rel_err_pos = accuracy.max_rel_error_pos;
    result.max_rel_err_acc = accuracy.max_rel_error_acc;

    // B. BENCHMARKS TEMPORALES
    result.cpu_serial = benchmarkCpuSerial(runs);

    NBodySystem sys_kernel = base_system;
    NBodySimulator sim_kernel(&sys_kernel, dt);

    double *d_u_ptr = nullptr;
    double *d_k_ptr = nullptr;
    if (energy_method > 0) {
        CUDA_CHECK(cudaMalloc(&d_u_ptr, sizeof(double)));
        CUDA_CHECK(cudaMalloc(&d_k_ptr, sizeof(double)));
    }

    result.gpu_kernel = benchmarkKernelOnly(
        sim_kernel, variant, energy_method, block_size, d_u_ptr, d_k_ptr, runs
    );

    if (d_u_ptr) CUDA_CHECK(cudaFree(d_u_ptr));
    if (d_k_ptr) CUDA_CHECK(cudaFree(d_k_ptr));

    result.gpu_end_to_end = benchmarkEndToEnd(variant, energy_method, block_size, runs);

    // C. MÉTRICAS DERIVADAS
    result.speedup_kernel = (result.gpu_kernel.mean_ms > 0.0) 
        ? (result.cpu_serial.mean_ms / result.gpu_kernel.mean_ms) : 0.0;

    result.speedup_e2e = (result.gpu_end_to_end.mean_ms > 0.0) 
        ? (result.cpu_serial.mean_ms / result.gpu_end_to_end.mean_ms) : 0.0;

    result.amdahl_f = (result.speedup_kernel > 0.0) ? (1.0 / result.speedup_kernel) : 1.0;

    if (!outputFile.empty()) {
        saveFullResultRow(outputFile, result);
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. SUITE COMPLETA EN UN SOLO PASE (SINGLE PASS)
// ─────────────────────────────────────────────────────────────────────────────

void Benchmark::runFullGpuTestSuite(
    const std::string& fullResultsFile,
    const std::string& scalingFile,
    const std::string& blockDimFile,
    int fixed_block_size,
    int fixed_n,
    int runs) 
{
    std::vector<int> particle_counts = {128, 256, 512, 1024, 2000}; 
    std::vector<int> variants = {0, 1};                            
    std::vector<int> block_sizes = {64, 128, 256, 512, 1024};             

    // 1. Inicializamos encabezados personalizados
    writeFullResultsHeader(fullResultsFile);
    writeScalingHeader(scalingFile);
    writeBlockDimHeader(blockDimFile);

    int total_tests = particle_counts.size() * variants.size() * block_sizes.size();
    int test_count = 0;

    std::cout << "\n======================================================\n";
    std::cout << "INICIANDO BENCHMARK GPU (SINGLE PASS - " << total_tests << " PRUEBAS)\n";
    std::cout << "======================================================\n";

    // 2. Un único producto cartesiano
    for (int n : particle_counts) {
        for (int var : variants) {
            for (int blk : block_sizes) {
                test_count++;
                std::cout << "[" << test_count << "/" << total_tests << "] N=" << n 
                          << ", Var=" << var << ", Blk=" << blk << "..." << std::endl;

                NBodySystem sys(1.0, 0.01);
                setupRandomSystem(sys, n);
                Benchmark bench(sys, steps, dt, runs);
                //AÑADIR OPCION DE QUE SI FIXED_BLOCK_SIZE O FIXED_N ES 0, SE IGNORE EL FILTRO Y SE GUARDE TODO
                //LO MISMO CON FIXED_N
                // Pasamos "" para controlar el guardado explícito
                CpuGpuComparison res = bench.runGpuComparisonTest("", n, var, blk, 0, runs);

                // A. Archivo Maestro siempre recibe la fila completa
                saveFullResultRow(fullResultsFile, res);

                // B. Filtro para scaling_analysis.dat
                if (blk == fixed_block_size) {
                    saveScalingRow(scalingFile, res);
                }

                // C. Filtro para blockdim_study.dat
                if (n == fixed_n) {
                    saveBlockDimRow(blockDimFile, res);
                }
            }
        }
    }

    std::cout << "\n[OK] Suite finalizada con éxito. Se generaron los 3 archivos en 1 solo pase.\n\n";
}