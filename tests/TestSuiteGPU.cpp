#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include "Benchmark.h"

TEST(BenchmarkGpuSuiteTest, Suite80PruebasKernelYEndToEnd) {
    std::string results_file = "gpu_suite_80_results.dat";
    
    // Limpiar ejecuciones anteriores
    std::remove(results_file.c_str());

    // Instanciar benchmark base (50 pasos por corrida para agilidad)
    NBodySystem base_sys(1.0, 0.01);
    Benchmark bench(base_sys, 50, 0.01, 3); 

    // Ejecutar la suite completa (40 configs x 2 modos = 80 mediciones)
    bench.runFullGpuTestSuite(results_file, 3);

    // Verificación de integridad del archivo de resultados
    std::ifstream file(results_file);
    ASSERT_TRUE(file.is_open()) << "Error: No se pudo generar el archivo de resultados.";

    std::string line;
    int total_lines = 0;
    while (std::getline(file, line)) {
        if (!line.empty()) total_lines++;
    }
    file.close();

    // 1 línea de cabecera + 50 filas (cada fila contiene el registro de Kernel y de E2E)
    EXPECT_EQ(total_lines, 51) << "El archivo debe contener exactamente la cabecera y las 50 filas registradas.";
}