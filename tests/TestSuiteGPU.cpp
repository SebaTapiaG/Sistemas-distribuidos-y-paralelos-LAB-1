#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include "Benchmark.h"

TEST(BenchmarkGpuSuiteTest, SinglePassFullGpuTestSuite) {
    std::string full_file = "test_gpu_suite_full.dat";
    std::string scaling_file = "test_gpu_suite_scaling.dat";
    std::string blockdim_file = "test_gpu_suite_blockdim.dat";
    
    // Limpiar ejecuciones anteriores
    std::remove(full_file.c_str());
    std::remove(scaling_file.c_str());
    std::remove(blockdim_file.c_str());

    // Instanciar benchmark base (10 pasos por corrida para rapidez en el test)
    NBodySystem base_sys(1.0, 0.01);
    Benchmark bench(base_sys, 10, 0.01, 2); 

    // Ejecutar la suite completa en Single Pass
    bench.runFullGpuTestSuite(
        full_file, 
        scaling_file, 
        blockdim_file, 
        256,  // fixed_block_size
        1024, // fixed_n
        2     // runs por test
    );

    // 1. Verificación del Archivo Maestro (fullResultsFile)
    std::ifstream f_full(full_file);
    ASSERT_TRUE(f_full.is_open()) << "Error: No se pudo generar el archivo maestro.";

    std::string line;
    int total_lines_full = 0;
    while (std::getline(f_full, line)) {
        if (!line.empty()) total_lines_full++;
    }
    f_full.close();

    // 1 línea de cabecera + 50 filas (5 Ns * 2 variantes * 5 block_sizes)
    EXPECT_EQ(total_lines_full, 51) << "El archivo maestro debe contener cabecera + 50 filas de resultados.";

    // 2. Verificación del Archivo de Escalabilidad (scalingFile -> block_size == 256)
    std::ifstream f_scal(scaling_file);
    ASSERT_TRUE(f_scal.is_open());
    int total_lines_scal = 0;
    while (std::getline(f_scal, line)) {
        if (!line.empty()) total_lines_scal++;
    }
    f_scal.close();
    // 1 cabecera + 10 filas (5 Ns * 2 variantes * 1 block_size filtrado)
    EXPECT_EQ(total_lines_scal, 11);

    // 3. Verificación del Archivo de Tamaño de Bloque (blockDimFile -> N == 1024)
    std::ifstream f_blk(blockdim_file);
    ASSERT_TRUE(f_blk.is_open());
    int total_lines_blk = 0;
    while (std::getline(f_blk, line)) {
        if (!line.empty()) total_lines_blk++;
    }
    f_blk.close();
    // 1 cabecera + 10 filas (1 N filtrado * 2 variantes * 5 block_sizes)
    EXPECT_EQ(total_lines_blk, 11);

    // Limpieza final de archivos de prueba
    std::remove(full_file.c_str());
    std::remove(scaling_file.c_str());
    std::remove(blockdim_file.c_str());
}