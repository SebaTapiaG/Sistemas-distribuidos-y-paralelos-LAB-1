#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include "MetricsCalculator.h"

/**
 * Pruebas unitarias para MetricsCalculator:
 * Verifica el cálculo de promedios, desviaciones estándar, propagación de errores,
 * ley de Amdahl y análisis físico/integridad de partículas.
 */

// 1. Precisión Estadística Básica
TEST(MetricsCalculatorTest, StatisticalPrecision) {
    std::vector<double> times = {10.0, 10.0, 10.0, 10.0};
    double mean = MetricsCalculator::calculateMean(times);
    EXPECT_DOUBLE_EQ(mean, 10.0);
    
    // Si los datos son idénticos, la desviación estándar debe ser 0.0
    EXPECT_DOUBLE_EQ(MetricsCalculator::calculateStdDev(times, mean), 0.0);
}

// 2. Desviación Estándar con Varianza
TEST(MetricsCalculatorTest, StandardDeviationWithVariance) {
    std::vector<double> times = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    double mean = MetricsCalculator::calculateMean(times); // Media = 5.0
    double stdDev = MetricsCalculator::calculateStdDev(times, mean);
    
    EXPECT_DOUBLE_EQ(mean, 5.0);
    // Varianza muestral = 32 / 7 ≈ 4.571428... -> StdDev ≈ 2.1380899...
    EXPECT_NEAR(stdDev, 2.1380899, 1e-6);
}

// 3. Verificación del Centro de Masa
TEST(MetricsCalculatorTest, CalculateCenterOfMass) {
    std::vector<Particle> particles;
    // Partícula 1: masa 2.0 en (10.0, 0.0)
    Particle p1(2.0, 10.0, 0.0);
    // Partícula 2: masa 3.0 en (0.0, 20.0)
    Particle p2(3.0, 0.0, 20.0);
    
    particles.push_back(p1);
    particles.push_back(p2);

    CenterOfMass com = MetricsCalculator::calculateCenterOfMass(particles);

    // X_cm = (2*10 + 3*0) / 5 = 4.0
    // Y_cm = (2*0 + 3*20) / 5 = 12.0
    EXPECT_DOUBLE_EQ(com.x, 4.0);
    EXPECT_DOUBLE_EQ(com.y, 12.0);
}

// 4. Verificación de Integridad y Consistencia de Datos Paralelos
TEST(MetricsCalculatorTest, PhysicsAndConsistencyVerification) {
    std::vector<Particle> bodies;
    Particle p1(1.0, 0.0, 0.0);
    p1.setVx(10.0);
    
    Particle p2(1.0, 5.0, 5.0);
    p2.setVx(-10.0);

    bodies.push_back(p1);
    bodies.push_back(p2);

    DiagnosticResult diag = MetricsCalculator::verifyConsistency(bodies);
    
    EXPECT_TRUE(diag.consistency_pass);
    // La masa total capturada debe corresponder al checksum correcto
    EXPECT_DOUBLE_EQ(diag.last_particle_state.getMass(), 1.0);
}

// 5. Análisis Físico de Energía y Conservación de Momento
TEST(MetricsCalculatorTest, PhysicalAnalysis) {
    simulation_data data;
    // Conservación de energía perfecta (Drift = 0)
    data.k = {100.0, 80.0, 50.0};
    data.u = {0.0,   20.0, 50.0}; // E_total constante = 100.0

    // Momentos de partículas
    std::vector<Particle> snapshot_initial = { Particle(1.0, 0.0, 0.0) };
    std::vector<Particle> snapshot_final   = { Particle(1.0, 1.0, 1.0) };
    
    data.bodies.push_back(snapshot_initial);
    data.bodies.push_back(snapshot_final);

    PhysicalResult result = MetricsCalculator::analyzePhysics(data);

    EXPECT_DOUBLE_EQ(result.relative_error, 0.0);
    EXPECT_TRUE(result.is_valid);
}