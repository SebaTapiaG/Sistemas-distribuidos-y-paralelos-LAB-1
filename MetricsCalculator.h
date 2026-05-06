#ifndef METRICS_CALCULATOR_H
#define METRICS_CALCULATOR_H

#include <vector>
#include <cmath>
#include <numeric>
#include <iostream>
#include "NBodySimulator.h" // Para acceder a simulation_data

struct PerformanceResult {
    double mean_time;
    double std_dev;
    double speedup;
    double serial_fraction; // f en la Ley de Amdahl
    double theorical_speedup;
    double sigma_speedup;
    double efficiency;
    double sigma_efficiency;

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

class MetricsCalculator {
    public:
        // Métricas de Rendimiento
        static double calculateMean(const std::vector<double>& times);
        static double calculateStdDev(const std::vector<double>& times, double mean);
        
        static PerformanceResult analyzePerformance(const std::vector<double>& t1_times, 
                                                const std::vector<double>& tp_times, 
                                                int num_threads);

        // Métricas Físicas
        static PhysicalResult analyzePhysics(const simulation_data& data, double tolerance = 1e-5);

        // Análisis Teórico
        static double estimateSerialFraction(double speedup, int p);
        static double estimateSerialFractionByTime(double t1, double tp);
        static double calculateTheoricalSpeedup(double f, int p);

        static MomentumResult calculateMomentum(const std::vector<Particle>& bodies);
};

#endif