#ifndef METRICS_CALCULATOR_H
#define METRICS_CALCULATOR_H

#include <vector>
#include <cmath>
#include <numeric>
#include <iostream>
#include <omp.h>
#include "NBodySimulator.h" // Para acceder a simulation_data
struct DiagnosticResult {
    double energy;
    int last_index;
    bool consistency_pass;
    Particle last_particle_state; 
};

struct MomentumResult {
    double px, py;
    double magnitude;
};

struct PhysicalResult {
    double initial_energy;
    double final_energy;
    double energy_drift;
    double relative_error;
    MomentumResult initial_momentum;
    MomentumResult final_momentum; 
    bool is_valid;
};
struct CenterOfMass {
    double x;
    double y;
    double total_mass;
};
// Estructura para reportar el resultado de la comparación de tolerancias
struct AccuracyResult {
    bool passed;
    double max_rel_error_pos;
    double max_rel_error_acc;
    int failed_components;
};

class MetricsCalculator {
    public:
        // Métricas de Rendimiento
        static double calculateMean(const std::vector<double>& times);
        static double calculateStdDev(const std::vector<double>& times, double mean);
        // Métricas Físicas
        static PhysicalResult analyzePhysics(const simulation_data& data, double tolerance = 1e-5);
        static DiagnosticResult verifyConsistency(const std::vector<Particle>& bodies);
        static MomentumResult calculateMomentum(const std::vector<Particle>& bodies);
        static CenterOfMass calculateCenterOfMass(const std::vector<Particle>& bodies);
        static AccuracyResult compareCpuGpuAccuracy(
        const std::vector<Particle>& cpu_bodies,
        const std::vector<Particle>& gpu_bodies,
        const std::vector<double>& gpu_ax,
        const std::vector<double>& gpu_ay,
        const std::vector<double>& cpu_ax,
        const std::vector<double>& cpu_ay,
        double rtol = 1e-4, 
        double atol = 1e-8);
};



#endif