#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <random>
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
TEST(MetricsCalculatorTest, CpuGpuToleranceValidation) {
    const int N = 100;
    const double dt = 0.01;
    
    // 1. Inicializar sistema CPU con datos aleatorios
    NBodySystem system_cpu(1.0, 0.01);
    std::mt19937 gen(42);
    std::uniform_real_distribution<> pos_dis(0.0, 100.0);
    std::uniform_real_distribution<> mass_dis(1.0, 10.0);

    for (int i = 0; i < N; ++i) {
        system_cpu.addParticle(Particle(mass_dis(gen), pos_dis(gen), pos_dis(gen)));
    }

    // 2. Duplicar sistema exacto para la GPU
    NBodySystem system_gpu = system_cpu;

    // 3. Ejecutar 1 paso de aceleraciones/integración en CPU
    NBodySimulator sim_cpu(&system_cpu, dt);
    sim_cpu.processBodies(1);

    // 4. Ejecutar 1 paso en GPU (fuerza la descarga de estado con record_frames=true o copyDeviceToHost)
    NBodySimulator sim_gpu(&system_gpu, dt);
    sim_gpu.processBodiesGpu(1, 0, 0, 128, true); // record_frames=true descarga posiciones/aceleraciones SoA -> AoS

    // 5. Obtener partículas
    const auto& cpu_particles = system_cpu.getBodies();
    const auto& gpu_particles = system_gpu.getBodies();

    // 6. Construir los vectores de aceleración reales calculados en este paso
    std::vector<double> cpu_ax, cpu_ay, gpu_ax, gpu_ay;
    cpu_ax.reserve(N); cpu_ay.reserve(N);
    gpu_ax.reserve(N); gpu_ay.reserve(N);

    for (int i = 0; i < N; ++i) {
        cpu_ax.push_back(cpu_particles[i].getAx());
        cpu_ay.push_back(cpu_particles[i].getAy());

        gpu_ax.push_back(gpu_particles[i].getAx());
        gpu_ay.push_back(gpu_particles[i].getAy());
    }

    // 7. Validar precisión con métricas usando aceleraciones REALES
    AccuracyResult acc = MetricsCalculator::compareCpuGpuAccuracy(
        cpu_particles, gpu_particles, 
        cpu_ax, cpu_ay, gpu_ax, gpu_ay, 
        1e-4, 1e-8
    );

    // Evaluación del test
    EXPECT_TRUE(acc.passed) 
        << "Divergencia detectada entre CPU y GPU en iteración 1.\n"
        << "  Max Error Relativo Posición: " << acc.max_rel_error_pos << "\n"
        << "  Max Error Relativo Aceleración: " << acc.max_rel_error_acc;
}