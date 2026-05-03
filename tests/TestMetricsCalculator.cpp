#include <gtest/gtest.h>
#include "MetricsCalculator.h"
#include <vector>
#include <cmath>

// 1. Prueba de cálculos estadísticos básicos (Tiempo)
TEST(MetricsCalculatorTest, BasicStatistics) {
    std::vector<double> times = {10.0, 12.0, 11.0, 9.0, 13.0};
    double mean = MetricsCalculator::calculateMean(times);
    
    // Promedio: 11.0
    EXPECT_DOUBLE_EQ(mean, 11.0);
    
    // Desviación estándar muestral: ~1.5811
    double stddev = MetricsCalculator::calculateStdDev(times, mean);
    EXPECT_NEAR(stddev, 1.58113883, 1e-7);
}

// 2. Prueba de análisis de rendimiento e incertidumbre
TEST(MetricsCalculatorTest, PerformanceAnalysis) {
    std::vector<double> t1_times = {100.0, 102.0, 98.0, 101.0, 99.0};
    std::vector<double> tp_times = {25.0, 26.0, 24.0, 25.5, 24.5};
    int p = 4;

    PerformanceResult res = MetricsCalculator::analyzePerformance(t1_times, tp_times, p);

    // Speedup esperado aprox: 100 / 25 = 4.0
    EXPECT_NEAR(res.speedup, 4.0, 0.2);
    // Eficiencia esperada aprox: 1.0
    EXPECT_NEAR(res.efficiency, 1.0, 0.1);
    // Verificar que la incertidumbre fue calculada (debe ser > 0)
    EXPECT_GT(res.sigma_speedup, 0.0);
}

// 3. NUEVO: Prueba de cálculo de Momento Lineal
TEST(MetricsCalculatorTest, MomentumCalculation) {
    std::vector<Particle> bodies;
    // Partícula 1: masa 2.0, vx 5.0, vy 0.0 -> Px = 10.0
    Particle p1(2.0, 0.0, 0.0);
    p1.setVx(5.0);
    p1.setVy(0.0);
    
    // Partícula 2: masa 1.0, vx -10.0, vy 5.0 -> Px = -10.0, Py = 5.0
    Particle p2(1.0, 10.0, 10.0);
    p2.setVx(-10.0);
    p2.setVy(5.0);
    
    bodies.push_back(p1);
    bodies.push_back(p2);

    MomentumResult res = MetricsCalculator::calculateMomentum(bodies);

    // Px total = 10.0 + (-10.0) = 0.0
    EXPECT_NEAR(res.px, 0.0, 1e-9);
    // Py total = 0.0 + 5.0 = 5.0
    EXPECT_NEAR(res.py, 5.0, 1e-9);
    // Magnitud = sqrt(0^2 + 5^2) = 5.0
    EXPECT_DOUBLE_EQ(res.magnitude, 5.0);
}

// 4. ACTUALIZADO: Prueba de validación física (Energía + Momento)
TEST(MetricsCalculatorTest, PhysicsValidationFull) {
    simulation_data data;
    
    // Simular 2 partículas en 2 pasos de tiempo
    Particle p_init(1.0, 0.0, 0.0);
    p_init.setVx(1.0);
    
    // Paso inicial
    data.k = {0.5}; // 1/2 * m * v^2
    data.u = {-0.2};
    data.bodies.push_back({p_init}); 

    // Paso final (conservativo)
    data.k.push_back(0.4);
    data.u.push_back(-0.1);
    data.bodies.push_back({p_init}); // Misma velocidad, momento se conserva

    PhysicalResult res = MetricsCalculator::analyzePhysics(data, 1e-5);
    
    // E_total inicial = 0.3, E_total final = 0.3. Delta = 0.
    EXPECT_TRUE(res.is_valid);
    EXPECT_NEAR(res.energy_drift, 0.0, 1e-9);
    EXPECT_NEAR(res.initial_momentum.magnitude, res.final_momentum.magnitude, 1e-9);
}

// 5. Prueba de la Ley de Amdahl
TEST(MetricsCalculatorTest, AmdahlAnalysis) {
    // Si con 4 hilos el speedup es 2, f debería ser 1/3
    double f = MetricsCalculator::estimateSerialFraction(2.0, 4);
    EXPECT_NEAR(f, 0.333333, 1e-5);
}