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
CenterOfMass MetricsCalculator::calculateCenterOfMass(const std::vector<Particle>& bodies) {
    double total_m = 0.0;
    double sum_mx = 0.0;
    double sum_my = 0.0;
    const int n = static_cast<int>(bodies.size());

    // Acumulación paralela mediante reducción de OpenMP
    #pragma omp parallel for reduction(+:total_m, sum_mx, sum_my)
    for (int i = 0; i < n; ++i) {
        double m = bodies[i].getMass();
        total_m += m;
        sum_mx += m * bodies[i].getX();
        sum_my += m * bodies[i].getY();
    }

    if (total_m <= 0.0) {
        return {0.0, 0.0, 0.0};
    }

    return {sum_mx / total_m, sum_my / total_m, total_m};
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
AccuracyResult MetricsCalculator::compareCpuGpuAccuracy(
    const std::vector<Particle>& cpu_bodies,
    const std::vector<Particle>& gpu_bodies,
    const std::vector<double>& gpu_ax,
    const std::vector<double>& gpu_ay,
    const std::vector<double>& cpu_ax,
    const std::vector<double>& cpu_ay,
    double rtol, 
    double atol) 
{
    AccuracyResult res{true, 0.0, 0.0, 0};
    size_t n = cpu_bodies.size();

    if (gpu_bodies.size() != n || cpu_ax.size() != n || gpu_ax.size() != n) {
        res.passed = false;
        return res;
    }

    auto check_tolerance = [rtol, atol](double val_gpu, double val_cpu, double& max_rel_err) {
        double diff = std::abs(val_gpu - val_cpu);
        double max_allowed = atol + rtol * std::abs(val_cpu);
        
        double rel_err = (std::abs(val_cpu) > 1e-12) ? (diff / std::abs(val_cpu)) : diff;
        if (rel_err > max_rel_err) max_rel_err = rel_err;

        return diff <= max_allowed;
    };

    for (size_t i = 0; i < n; ++i) {
        // 1. Validar Posiciones (X, Y)
        bool px = check_tolerance(gpu_bodies[i].getX(), cpu_bodies[i].getX(), res.max_rel_error_pos);
        bool py = check_tolerance(gpu_bodies[i].getY(), cpu_bodies[i].getY(), res.max_rel_error_pos);

        // 2. Validar Aceleraciones (AX, AY)
        bool ax = check_tolerance(gpu_ax[i], cpu_ax[i], res.max_rel_error_acc);
        bool ay = check_tolerance(gpu_ay[i], cpu_ay[i], res.max_rel_error_acc);

        if (!px || !py || !ax || !ay) {
            res.passed = false;
            res.failed_components++;
        }
    }

    return res;
}