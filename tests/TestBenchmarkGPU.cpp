#include <gtest/gtest.h>
#include <fstream>
#include <cmath>
#include <vector>
#include <random>
#include "Benchmark.h"
#include "NBodySystem.h"

// ============================================================================
// 1. TEST DE MÉTODOS ESTADÍSTICOS Y FÓRMULAS
// ============================================================================

TEST(BenchmarkStatsTest, CalculateStatsCorrectness) {
    // Conjunto de datos conocido: [2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0]
    // Media = 5.0, Varianza muestral = 4.5, Desviación estándar ≈ 2.121320
    std::vector<double> sample = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};

    MeasurementResult res = Benchmark::calculateStats(sample);

    EXPECT_NEAR(res.mean_ms, 5.0, 1e-5);
    EXPECT_NEAR(res.stddev_ms, 2.138089935, 1e-5);
}

TEST(BenchmarkStatsTest, EstimateSerialFractionAmdahl) {
    // Si P = 4 y Speedup = 2.0:
    // f = ((1/2) - (1/4)) / (1 - (1/4)) = (0.25) / (0.75) = 1/3 ≈ 0.333333
    double f = Benchmark::estimateSerialFraction(2.0, 4);
    EXPECT_NEAR(f, 1.0 / 3.0, 1e-5);

    // Si P = 1, la fracción serial debe ser 1.0 por definición
    EXPECT_DOUBLE_EQ(Benchmark::estimateSerialFraction(1.5, 1), 1.0);
}

TEST(BenchmarkStatsTest, CalculateTheoricalSpeedup) {
    // Si f = 0.2 y P = 8:
    // Sp = 1 / (0.2 + (0.8 / 8)) = 1 / (0.2 + 0.1) = 1 / 0.3 ≈ 3.333333
    double speedup = Benchmark::calculateTheoricalSpeedup(0.2, 8);
    EXPECT_NEAR(speedup, 10.0 / 3.0, 1e-5);
}

// ============================================================================
// 2. FIXTURE PARA INTEGRACIÓN CPU/GPU CON NBODYSYSTEM
// ============================================================================

class BenchmarkIntegrationTest : public ::testing::Test {
protected:
    NBodySystem test_system;
    int num_particles = 128;
    int steps = 10;
    double dt = 0.01;

    BenchmarkIntegrationTest() : test_system(1.0, 0.01) {}

    void SetUp() override {
        // Inicializamos un sistema pequeño para ejecuciones rápidas en las pruebas
        std::mt19937 gen(42);
        std::uniform_real_distribution<> pos(0, 500);
        std::uniform_real_distribution<> mass(1, 50);

        for (int i = 0; i < num_particles; ++i) {
            test_system.addParticle(Particle(mass(gen), pos(gen), pos(gen)));
        }
    }
};

// Test para la ejecución serial en CPU
TEST_F(BenchmarkIntegrationTest, CpuSerialBenchmarkRunsCorrectly) {
    int runs = 3;
    Benchmark bench(test_system, steps, dt, runs);

    MeasurementResult res = bench.benchmarkCpuSerial(runs);

    // Comprobar que los tiempos calculados son positivos
    EXPECT_GT(res.mean_ms, 0.0);
    EXPECT_GE(res.stddev_ms, 0.0);
}

// Test para la comparación completa entre CPU y GPU (Individual)
TEST_F(BenchmarkIntegrationTest, RunGpuComparisonTestAndExport) {
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);

    if (err != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "Saltando test de comparación GPU: No hay GPU o driver NVIDIA presente.";
    }
    std::string test_output_file = "test_gpu_comparison.dat";
    
    // Eliminar archivo si ya existía antes de la prueba
    std::remove(test_output_file.c_str());

    Benchmark bench(test_system, steps, dt, 3);
    
    // Escribir cabecera usando el método unificado
    Benchmark::writeFullResultsHeader(test_output_file);

    // Ejecutar test de comparación (variante 0, block_size 128, runs 3, rtol 1e-4, atol 1e-8)
    CpuGpuComparison comp = bench.runGpuComparisonTest(
        test_output_file, num_particles, 0, 128, 0, 3, 1e-4, 1e-8
    );

    // Validar resultados de la estructura
    EXPECT_EQ(comp.num_particles, num_particles);
    EXPECT_GT(comp.cpu_serial.mean_ms, 0.0);
    EXPECT_GT(comp.gpu_kernel.mean_ms, 0.0);
    EXPECT_GT(comp.gpu_end_to_end.mean_ms, 0.0);
    EXPECT_GT(comp.speedup_kernel, 0.0);
    EXPECT_GT(comp.speedup_e2e, 0.0);
    EXPECT_TRUE(comp.accuracy_pass);

    // Comprobar que el archivo se creó correctamente y tiene contenido
    std::ifstream file(test_output_file);
    ASSERT_TRUE(file.is_open());
    
    std::string line;
    int line_count = 0;
    while (std::getline(file, line)) {
        if (!line.empty()) line_count++;
    }
    file.close();

    // 1 línea de cabecera + 1 línea de resultados
    EXPECT_EQ(line_count, 2);

    // Limpieza
    std::remove(test_output_file.c_str());
}

// Test para el test de escalabilidad OpenMP (runScalabilityTest)
TEST_F(BenchmarkIntegrationTest, RunScalabilityTestDiagnostics) {
    std::string diag_file = "execution_integrity.dat";
    std::remove(diag_file.c_str());

    Benchmark bench(3, steps, dt, 42);

    // Ejecutar test de escalabilidad con diagnósticos habilitados
    bench.runScalabilityTest(
        2,              // max_threads
        num_particles,  // num_particles
        1,              // task_type (Parallel For)
        0,              // sync_type (atomic)
        0,              // energy_method (reduce)
        0,              // schedule_type
        16,             // chunk_size
        1.0,            // G
        0.01,           // epsilon
        true,           // perform_diagnostics = true
        0               // mode
    );

    // Verificar que generó el reporte de diagnóstico
    std::ifstream file(diag_file);
    EXPECT_TRUE(file.is_open());
    file.close();

    // Limpieza de archivos secundarios generados por runScalabilityTest
    std::remove(diag_file.c_str());
    std::remove("bench_results.dat");
    std::remove("scaling_analysis.dat");
    std::remove("amdahl_comparison.dat");
}