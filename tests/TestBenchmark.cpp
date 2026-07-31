#include <gtest/gtest.h>
#include <fstream>
#include <cmath>
#include "Benchmark.h"
#include "NBodySystem.h"

/**
 * Pruebas de integración y unitarias para Benchmark.
 */

class BenchmarkTest : public ::testing::Test {
protected:
    // Limpieza de archivos generados por las ejecuciones de prueba
    void TearDown() override {
        std::remove("bench_results.dat");
        std::remove("scaling_analysis.dat");
        std::remove("amdahl_comparison.dat");
        std::remove("execution_integrity.dat");
    }
};

// 1. Inicialización Determinista del Sistema Aleatorio
TEST_F(BenchmarkTest, SetupRandomSystemDeterminism) {
    Benchmark bm1(1, 10, 0.01, 42);
    Benchmark bm2(1, 10, 0.01, 42); // Misma semilla
    
    NBodySystem sys1(1.0, 0.1);
    NBodySystem sys2(1.0, 0.1);

    bm1.setupRandomSystem(sys1, 15);
    bm2.setupRandomSystem(sys2, 15);

    EXPECT_EQ(sys1.getBodies().size(), 15);
    EXPECT_EQ(sys2.getBodies().size(), 15);

    // Ambas inicializaciones deben generar la misma masa y posición
    EXPECT_DOUBLE_EQ(sys1.getBodies()[0].getMass(), sys2.getBodies()[0].getMass());
    EXPECT_DOUBLE_EQ(sys1.getBodies()[0].getX(), sys2.getBodies()[0].getX());
}

// 2. Despeje de la Ley de Amdahl (Fracción Serial)
TEST_F(BenchmarkTest, EstimateSerialFraction) {
    Benchmark bm(1, 10, 0.01, 42);

    // Para 1 hilo, f siempre debe ser 1.0
    EXPECT_DOUBLE_EQ(bm.estimateSerialFraction(1.0, 1), 1.0);

    // Si con 4 hilos el Speedup es perfecto (4.0), f debe ser 0.0
    EXPECT_DOUBLE_EQ(bm.estimateSerialFraction(4.0, 4), 0.0);

    // Si con 2 hilos el Speedup es 1.6:
    // f = ((1/1.6) - (1/2)) / (1 - 1/2) = (0.625 - 0.5) / 0.5 = 0.25
    EXPECT_NEAR(bm.estimateSerialFraction(1.6, 2), 0.25, 1e-6);
}

// 3. Estimación de Fracción Serial por Tiempo (Fórmula deductiva)
TEST_F(BenchmarkTest, EstimateSerialFractionByTime) {
    Benchmark bm(1, 10, 0.01, 42);

    // T1 = 100s, Tp (para 4 hilos) = 25s -> Speedup perfecto -> f = 0.0
    double f1 = bm.estimateSerialFractionByTime(100.0, 25.0, 4);
    EXPECT_DOUBLE_EQ(f1, 0.0);

    // T1 = 100s, Tp (para 2 hilos) = 62.5s -> f = 0.25
    double f2 = bm.estimateSerialFractionByTime(100.0, 62.5, 2);
    EXPECT_NEAR(f2, 0.25, 1e-6);
}

// 4. Comparación de Modos de Análisis (Modo Tradicional vs FirstPrivate)
TEST_F(BenchmarkTest, PerformanceAnalysisModesConsistency) {
    Benchmark bm(5, 10, 0.01, 42);

    std::vector<double> t1 = {100.0, 102.0, 98.0, 101.0, 99.0};
    std::vector<double> tp = {50.0,  51.0,  49.0,  50.5,  49.5};
    std::vector<double> ts = {1.0,   1.1,   0.9,   1.0,   1.0};

    // Modo 0: Tradicional
    PerformanceResult res0 = bm.analyzePerformance(t1, tp, ts, 2, 0);
    // Modo 1: OpenMP FirstPrivate
    PerformanceResult res1 = bm.analyzePerformance(t1, tp, ts, 2, 1);

    // Ambos modos deben entregar resultados idénticos dentro del margen de precisión
    EXPECT_DOUBLE_EQ(res0.mean_time, res1.mean_time);
    EXPECT_DOUBLE_EQ(res0.speedup, res1.speedup);
    EXPECT_DOUBLE_EQ(res0.efficiency, res1.efficiency);
    EXPECT_NEAR(res0.std_dev, res1.std_dev, 1e-7);
    EXPECT_NEAR(res0.sigma_speedup, res1.sigma_speedup, 1e-7);
}

// 5. Flujo Completo de Prueba de Escalabilidad y Salida de Archivos
TEST_F(BenchmarkTest, RunScalabilityTestAndFileGeneration) {
    // 2 repeticiones, 3 pasos
    Benchmark bm(2, 3, 0.01, 123);

    // Ejecución rápida con max_threads = 2, 10 partículas
    simulation_data data = bm.runScalabilityTest(
        2, 10, 1, 0, 0, 0, 1, 1.0, 0.1, true, 0
    );

    // 1. Comprobar que los vectores de datos no estén vacíos
    EXPECT_FALSE(data.k.empty());
    EXPECT_FALSE(data.u.empty());

    // 2. Comprobar que los archivos de análisis de escalabilidad hayan sido generados
    std::ifstream scalFile("scaling_analysis.dat");
    EXPECT_TRUE(scalFile.good());

    std::string header;
    std::getline(scalFile, header);
    
    // Verificar que las columnas esperadas existen en la cabecera del archivo
    EXPECT_NE(header.find("SyncType"), std::string::npos);
    EXPECT_NE(header.find("Speedup"), std::string::npos);
    EXPECT_NE(header.find("SigmaSpeedup"), std::string::npos);
    scalFile.close();

    // 3. Comprobar la creación del log de integridad diagnóstica
    std::ifstream diagFile("execution_integrity.dat");
    EXPECT_TRUE(diagFile.good());
    diagFile.close();
}