#include "MetricsCalculator.h"
// Método para calcular la media de un vector de tiempos
double MetricsCalculator::calculateMean(const std::vector<double>& times) {
    if (times.empty()){
        return 0.0;
    }
    double sum = std::accumulate(times.begin(), times.end(), 0.0);
    double mean = sum / times.size();
    return mean;
}
// Método para calcular la desviación estándar de un vector de tiempos dado su media
double MetricsCalculator::calculateStdDev(const std::vector<double>& times, double mean) {
    if (times.size() < 2){
        return 0.0;
    }
    double sq_sum = 0.0;
    for (double t : times){
        sq_sum += (t - mean) * (t - mean);
    }
    double stddev = std::sqrt(sq_sum / (times.size() - 1)); 
    return stddev;
}
// Método que encapsula el análisis de rendimiento, permitiendo elegir entre el método tradicional o 
// el basado en firstprivate para el cálculo de desviaciones estándar.
PerformanceResult MetricsCalculator::analyzePerformance(const std::vector<double>& t1_times, 
                                                      const std::vector<double>& tp_times, 
                                                      const std::vector<double>& ts_times,
                                                      int num_threads, int mode) {
    if (mode == 0) {
        return analyzePerformance(t1_times, tp_times, ts_times, num_threads);
    } else {
        return analyzePerformanceFirstPrivate(t1_times, tp_times, ts_times, num_threads);
    }
}
//Método específico para el análisis de rendimiento de manera serial, calculando métricas como el speedup, la fracción serial y la eficiencia,
//  junto con sus respectivas desviaciones estándar para una interpretación estadística robusta.
PerformanceResult MetricsCalculator::analyzePerformance(const std::vector<double>& t1_times, 
                                                      const std::vector<double>& tp_times,
                                                      const std::vector<double>& ts_times,
                                                      int num_threads) {
    PerformanceResult res;
    
    double t1 = calculateMean(t1_times);
    double sigma_t1 = calculateStdDev(t1_times, t1);
    
    double tp = calculateMean(tp_times);
    double sigma_tp = calculateStdDev(tp_times, tp);

    double ts = calculateMean(ts_times);
    double sigma_ts = calculateStdDev(ts_times, ts);
    double tf= estimateSerialFractionByTime(t1, tp);
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
    double rel_err_ts = (ts > 0) ? (sigma_ts / ts) : 0;
    res.sigma_speedup = res.speedup * std::sqrt(rel_err_t1 * rel_err_t1 + rel_err_tp * rel_err_tp);
    res.efficiency = res.speedup / num_threads;
    res.sigma_efficiency = res.sigma_speedup / num_threads;
    res.sigma_serial_fraction = res.serial_fraction*(std::sqrt(rel_err_t1 * rel_err_t1 + rel_err_tp * rel_err_tp));
    res.sigma_theorical_speedup =  res.theorical_speedup*(std::sqrt(rel_err_ts * rel_err_ts + rel_err_tp * rel_err_tp));

    return res;
}
// Método específico para el análisis de rendimiento utilizando firstprivate, que paraleliza el cálculo de las 
// desviaciones estándar para mejorar la eficiencia en la interpretación estadística de los resultados.
PerformanceResult MetricsCalculator::analyzePerformanceFirstPrivate(
    const std::vector<double>& t1_times, 
    const std::vector<double>& tp_times,
    const std::vector<double>& ts_times,
    int num_threads) 
{
    std::cout<< "Analisis de rendimiento utilizando FirstPrivate para el cálculo de desviaciones estándar..." << std::endl;
    PerformanceResult res;
    double t1 = calculateMean(t1_times);
    double tp = calculateMean(tp_times);
    double ts = calculateMean(ts_times);
    
    int n = tp_times.size();
    double t1_stdDev = 0.0;
    double tp_stdDev = 0.0;
    double ts_stdDev = 0.0;
    double speedup = t1 / tp;
    //se paraleliza el cálculo de las desviaciones estándar usando firstprivate 
    //para las medias y reduction para acumular las sumas de cuadrados de las diferencias
    #pragma omp parallel for firstprivate(t1, tp, ts) reduction(+:t1_stdDev, tp_stdDev, ts_stdDev)
    for (int i = 0; i < n; ++i) {
        t1_stdDev += (t1_times[i] - t1) * (t1_times[i] - t1);
        tp_stdDev += (tp_times[i] - tp) * (tp_times[i] - tp);
        ts_stdDev += (ts_times[i] - ts) * (ts_times[i] - ts);
    }
    double sigma_t1 = std::sqrt(t1_stdDev / (n - 1));
    double sigma_tp = std::sqrt(tp_stdDev / (n - 1));
    double sigma_ts = std::sqrt(ts_stdDev / (n - 1));
    double tf= estimateSerialFractionByTime(t1, tp);
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
    double rel_err_ts = (ts > 0) ? (sigma_ts / ts) : 0;
    res.sigma_speedup = res.speedup * std::sqrt(rel_err_t1 * rel_err_t1 + rel_err_tp * rel_err_tp);
    res.efficiency = res.speedup / num_threads;
    res.sigma_efficiency = res.sigma_speedup / num_threads;
    res.sigma_serial_fraction = res.serial_fraction*(std::sqrt(rel_err_t1 * rel_err_t1 + rel_err_tp * rel_err_tp));
    res.sigma_theorical_speedup =  res.theorical_speedup*(std::sqrt(rel_err_ts * rel_err_ts + rel_err_tp * rel_err_tp));
    // ... (puedes incluir aquí el resto de tus cálculos de sigma)
    return res;
}
//no se usan
MomentumResult MetricsCalculator::calculateMomentum(const std::vector<Particle>& bodies) {
    double total_px = 0.0;
    double total_py = 0.0;
    for (const auto& p : bodies) {
        total_px += p.getMass() * p.getVx();
        total_py += p.getMass() * p.getVy();
    }

    return {total_px, total_py, std::sqrt(total_px * total_px + total_py * total_py)};
}

PhysicalResult MetricsCalculator::analyzePhysics(const simulation_data& data, double tolerance) {
    PhysicalResult res;
    
    // Energía
    double e_initial = data.k.front() + data.u.front();
    double e_final = data.k.back() + data.u.back();
    res.energy_drift = std::abs(e_final - e_initial);
    res.relative_error = (std::abs(e_initial) > 1e-15) ? (res.energy_drift / std::abs(e_initial)) : res.energy_drift;

    // Momento (usando los snapshots de partículas que guarda simulation_data)
    res.initial_momentum = calculateMomentum(data.bodies.front());
    res.final_momentum = calculateMomentum(data.bodies.back());

    // El sistema es válido si la energía se conserva Y el momento no varía drásticamente
    bool energy_ok = (res.relative_error <= tolerance);
    bool momentum_ok = std::abs(res.final_momentum.magnitude - res.initial_momentum.magnitude) < tolerance;
    
    res.is_valid = (energy_ok && momentum_ok);
    
    return res;
}
//Metodo de calculo de la fracción serial a partir del speedup medido, basado en la Ley de Amdahl.
double MetricsCalculator::estimateSerialFraction(double speedup, int p) {
    if (p <= 1){
        return 1.0;
    }
    // Basado en Ley de Amdahl despejada para 'f'
    double f = ((1.0 / speedup) - (1.0 / (double)p)) / (1.0 - (1.0 / (double)p));
    return (f < 0) ? 0 : f; // Ajuste por ruido experimental
}
// Metodo de estimación de la fracción serial a partir de los tiempos medidos, 
double MetricsCalculator::estimateSerialFractionByTime(double t1, double tp){
    if (t1 <= 0 || tp <= 0){
        return 1.0;
    }
    double f = t1/(t1+tp);
    return (f < 0) ? 0 : f; // Ajuste por ruido experimental
}
// Metodo de calculo del speedup teorico segun la ley de amdahl, utilizando la fraccion serial estimada a partir de los tiempos medidos.
double MetricsCalculator::calculateTheoricalSpeedup(double f, int p) {
    if (p <= 1){
        return 1.0;
    }
    return 1.0 / (f + ((1.0 - f) / (double)p));
}
// Método para verificar la consistencia física del sistema, 
// calculando la energía cinética total y un snapshot de la última partícula procesada.
DiagnosticResult MetricsCalculator::verifyConsistency(const std::vector<Particle>& bodies) {
    double total_k = 0.0;
    int last_idx = -1;
    int n = static_cast<int>(bodies.size());
    
    double m_last = 0, x_last = 0, y_last = 0;

    #pragma omp parallel for reduction(+:total_k) lastprivate(last_idx, m_last, x_last, y_last)
    for (int i = 0; i < n; ++i) {
        double m = bodies[i].getMass();
        total_k += 0.5 * m * (bodies[i].getVx()*bodies[i].getVx() + bodies[i].getVy()*bodies[i].getVy());
        
        last_idx = i;
        m_last = m;
        x_last = bodies[i].getX();
        y_last = bodies[i].getY();
    }

    // Snapshot obtenido mediante lastprivate (Modelo Paralelo)
    Particle parallel_snapshot(m_last, x_last, y_last);
    

    DiagnosticResult res = {total_k, last_idx, (last_idx == n - 1), parallel_snapshot};
    
    return res;
}