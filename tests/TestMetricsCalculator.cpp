#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include "MetricsCalculator.h"

/**
 * Test enfocado en la precisión de las métricas de rendimiento.
 * Como encargado de Benchmark, este archivo asegura que tus reportes
 * de Speedup y Eficiencia sean estadísticamente correctos.
 */

TEST(MetricsCalculatorTest, StatisticalPrecision) {
    // Caso con varianza conocida
    std::vector<double> times = {10.0, 10.0, 10.0, 10.0};
    double mean = MetricsCalculator::calculateMean(times);
    EXPECT_DOUBLE_EQ(mean, 10.0);
    
    // StdDev de valores idénticos debe ser 0
    EXPECT_DOUBLE_EQ(MetricsCalculator::calculateStdDev(times, mean), 0.0);
}

TEST(MetricsCalculatorTest, AmdahlLawValidation) {
    // Si el tiempo serial (T1) es 100 y con 4 hilos (Tp) es 25:
    // Speedup = 4.0, Eficiencia = 1.0, Fracción Serial = 0.0
    std::vector<double> t1 = {100.0};
    std::vector<double> tp = {25.0};
    std::vector<double> ts = {0.0}; // Asumimos ts despreciable para este test
    int threads = 4;

    PerformanceResult res = MetricsCalculator::analyzePerformance(t1, tp, ts, threads);

    EXPECT_DOUBLE_EQ(res.speedup, 4.0);
    EXPECT_DOUBLE_EQ(res.efficiency, 1.0);
    EXPECT_NEAR(res.serial_fraction, 0.0, 1e-7);
}

TEST(MetricsCalculatorTest, ErrorPropagationSpeedup) {
    // Test de propagación de errores: S = T1 / Tp
    // sigma_S = S * sqrt((sigma_T1/T1)^2 + (sigma_Tp/Tp)^2)
    std::vector<double> t1 = {90.0, 110.0}; // Media 100, StdDev ~14.14
    std::vector<double> tp = {45.0, 55.0}; // Media 50, StdDev ~7.07
    std::vector<double> ts = {0.0, 0.0};
    
    PerformanceResult res = MetricsCalculator::analyzePerformance(t1, tp, ts, 2);
    
    EXPECT_DOUBLE_EQ(res.speedup, 2.0);
    // La incertidumbre debe ser mayor que cero
    EXPECT_GT(res.sigma_speedup, 0.0);
    // La eficiencia con 2 hilos y speedup 2 debe ser 1.0
    EXPECT_DOUBLE_EQ(res.efficiency, 1.0);
}

TEST(MetricsCalculatorTest, PhysicsConsistency) {
    // Verificamos que la reducción de energía y el lastprivate funcionen
    std::vector<Particle> bodies;
    bodies.push_back(Particle(1.0, 0.0, 0.0));
    bodies.back().setVx(10.0);
    
    bodies.push_back(Particle(1.0, 5.0, 5.0));
    bodies.back().setVx(-10.0);

    DiagnosticResult diag = MetricsCalculator::verifyConsistency(bodies);
    
    EXPECT_TRUE(diag.consistency_pass);
    // El lastprivate debe haber capturado la última partícula (índice 1)
    EXPECT_EQ(diag.last_index, 1);
}