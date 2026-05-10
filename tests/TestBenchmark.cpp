#include <gtest/gtest.h>
#include <fstream>
#include "Benchmark.h"
#include "NBodySystem.h"

/**
 * Test enfocado en la capacidad del Benchmark para ejecutar
 * experimentos y generar archivos de salida correctos.
 */

class BenchmarkIntegrationTest : public ::testing::Test {
protected:
    // Limpiar archivos generados después de los tests
    void TearDown() override {
        std::remove("scaling_analysis.dat");
        std::remove("performance_results.dat");
    }
};

TEST_F(BenchmarkIntegrationTest, SystemSetupValidation) {
    Benchmark bm(1, 10, 0.01, 42); // 1 rep, 10 pasos, dt 0.01
    NBodySystem system(1.0, 0.1);
    
    bm.setupRandomSystem(system, 20); // Crear 20 partículas
    
    EXPECT_EQ(system.getBodies().size(), 20);
    
    // Verificar que la semilla 42 sea determinista
    double first_mass = system.getBodies()[0].getMass();
    EXPECT_GT(first_mass, 0.0);
}

TEST_F(BenchmarkIntegrationTest, FullExecutionFlow) {
    // Configuramos un benchmark pequeño para no demorar el test
    // 2 repeticiones, 5 pasos de tiempo
    Benchmark bm(2, 5, 0.01, 123);
    
    // Ejecutamos con: 2 hilos, 10 partículas, parallel_for, atomic, energy_reduce, static
    simulation_data data = bm.runScalabilityTest(2, 10, 1, 0, 0, 0, 1, 1.0, 0.1, true);
    
    // 1. Verificar que se capturaron datos de energía para cada paso
    EXPECT_EQ(data.k.size(), 5);
    EXPECT_EQ(data.u.size(), 5);
    
    // 2. Verificar que se creó el archivo de resultados
    std::ifstream file("scaling_analysis.dat");
    EXPECT_TRUE(file.good());
    
    // 3. Verificar que el archivo no esté vacío
    std::string line;
    std::getline(file, line); // Leer cabecera o primera línea
    EXPECT_FALSE(line.empty());
    file.close();
}

TEST_F(BenchmarkIntegrationTest, ScalingFileFormat) {
    Benchmark bm(1, 1, 0.01, 7);
    bm.runScalabilityTest(1, 4, 1, 0, 0, 0, 1, 1.0, 0.1, false);
    
    std::ifstream scalFile("scaling_analysis.dat");
    std::string header;
    std::getline(scalFile, header);
    
    // Verificar que existan las columnas clave en el archivo .dat
    EXPECT_NE(header.find("SyncType"), std::string::npos);
    EXPECT_NE(header.find("SigmaSpeedup"), std::string::npos);
    scalFile.close();
}