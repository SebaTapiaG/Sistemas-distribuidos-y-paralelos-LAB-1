#include <iostream>
#include <string>
#include <vector>
#include <random>
#include "Particle.h"
#include "NBodySystem.h"
#include "NBodySimulator.h"
#include "Visualizer.h"
#include "Benchmark.h"

using namespace std;
/**
 * Muestra el uso de la CLI del programa
 */
void printUsage(char* programName) {
    cout << "=== SIMULADOR N-BODY (GPU / CUDA) ===" << endl;
    cout << "Uso:" << endl;
    cout << "  1. Modo Suite Completa (Single Pass):" << endl;
    cout << "     " << programName << " -suite [runs] [fixed_block_size] [fixed_n]" << endl;
    cout << endl;
    cout << "  2. Modo Test Unico:" << endl;
    cout << "     " << programName << " -test [N] [variant] [block_size] [runs]" << endl;
    cout << endl;
    cout << "  3. Modo Simulacion / Visualizacion:" << endl;
    cout << "     " << programName << " -sim [N] [steps] [variant] [block_size]" << endl;
    cout << "======================================" << endl;
}

int main(int argc, char* argv[]) {
    // Parámetros físicos base
    const double G = 1.0;
    const double epsilon = 0.01;
    const double dt = 0.01;
    const unsigned int global_seed = 42;

    // Ayuda si se solicita
    if (argc > 1 && (string(argv[1]) == "-h" || string(argv[1]) == "--help")) {
        printUsage(argv[0]);
        return 0;
    }

    // -------------------------------------------------------------------------
    // MODO 1: SUITE COMPLETA DE BENCHMARKS (-suite)
    // -------------------------------------------------------------------------
    if (argc > 1 && string(argv[1]) == "-suite") {
        cout << ">>> Iniciando Suite Completa de Benchmarking GPU <<<" << endl;

        int runs             = (argc > 2) ? stoi(argv[2]) : 10;
        int fixed_block_size = (argc > 3) ? stoi(argv[3]) : 0;
        int fixed_n          = (argc > 4) ? stoi(argv[4]) : 0;

        cout << "Configuracion:" << endl;
        cout << " - Repeticiones (runs) : " << runs << endl;
        cout << " - BlockSize para Scaling: " << fixed_block_size << endl;
        cout << " - N para BlockDim Study : " << fixed_n << endl;

        // Instanciamos el Benchmark base
        Benchmark bm(runs, 150, dt, global_seed);

        // Ejecuta el barrido y genera los 3 archivos .dat en un solo pase
        bm.runFullGpuTestSuite(
            "benchmark_results.dat",
            "scaling_analysis.dat",
            "blockdim_study.dat",
            fixed_block_size,
            fixed_n,
            runs
        );

        cout << ">>> Suite de Benchmarks completada con exito. <<<" << endl;
        return 0;
    }

    // -------------------------------------------------------------------------
    // MODO 2: TEST ÚNICO / CUSTOM (-test)
    // -------------------------------------------------------------------------
    if (argc > 1 && string(argv[1]) == "-test") {
        cout << ">>> Ejecutando Test Unico CPU vs GPU <<<" << endl;

        int num_particles = (argc > 2) ? stoi(argv[2]) : 1024;
        int variant       = (argc > 3) ? stoi(argv[3]) : 0;
        int block_size    = (argc > 4) ? stoi(argv[4]) : 256;
        int runs          = (argc > 5) ? stoi(argv[5]) : 10;
        int energy_method = 0; // Por defecto

        cout << "Configuracion del Test:" << endl;
        cout << " - Particulas (N) : " << num_particles << endl;
        cout << " - Variante GPU   : " << variant << endl;
        cout << " - Block Size     : " << block_size << endl;
        cout << " - Corridas (runs): " << runs << endl;

        NBodySystem sys(G, epsilon);
        Benchmark bm(sys, 150, dt, runs);
        bm.setupRandomSystem(sys, num_particles);

        // Ejecuta la prueba individual con validación de tolerancias
        CpuGpuComparison result = bm.runGpuComparisonTest(
            "single_test_result.dat",
            num_particles,
            variant,
            block_size,
            energy_method,
            runs
        );

        cout << "\n--- RESUMEN DEL TEST ---" << endl;
        cout << " CPU Serial   : " << result.cpu_serial.mean_ms << " ms" << endl;
        cout << " GPU Kernel   : " << result.gpu_kernel.mean_ms << " ms (Speedup: " << result.speedup_kernel << "x)" << endl;
        cout << " GPU End-2-End: " << result.gpu_end_to_end.mean_ms << " ms (Speedup: " << result.speedup_e2e << "x)" << endl;
        cout << " Validacion   : " << (result.accuracy_pass ? "PASO [OK]" : "FALLO [FAIL]") << endl;
        
        return 0;
    }

    // -------------------------------------------------------------------------
    // MODO 3: SIMULACIÓN / VISUALIZACIÓN (-sim o Ejecución por Defecto)
    // -------------------------------------------------------------------------
    cout << ">>> Ejecutando Simulacion N-Body (Modo Visualizacion) <<<" << endl;

    int num_particles = (argc > 2) ? stoi(argv[2]) : 500;
    int steps         = (argc > 3) ? stoi(argv[3]) : 5000;
    int variant       = (argc > 4) ? stoi(argv[4]) : 0;
    int block_size    = (argc > 5) ? stoi(argv[5]) : 256;
    int energy_method = (argc > 6) ? stoi(argv[6]) : 0;

    // Inicialización del sistema físico
    NBodySystem system(G, epsilon);
    mt19937 gen(global_seed);
    uniform_real_distribution<> pos_dis(0, 1000);
    uniform_real_distribution<> mass_dis(1, 100);

    for (int i = 0; i < num_particles; ++i) {
        system.addParticle(Particle(mass_dis(gen), pos_dis(gen), pos_dis(gen)));
    }

    NBodySimulator simulator(&system, dt);

    // Ejecutamos en GPU registrando fotogramas (record_frames = true)
    simulation_data data = simulator.processBodiesGpu(steps, variant, energy_method, block_size, true);

    // Exportar datos de la simulación
    Visualizer vis;
    vis.exportarEnergia(data, "energy_timeseries.dat");     // Escribe energy_timeseries.dat
    vis.exportarTrayectorias(data, "trajectories.dat");     // Escribe trajectories.dat

    cout << "Simulacion finalizada. Datos procesados correctamente." << endl;
    return 0;
}