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
 * Función de ayuda para imprimir el uso del programa
 */
void printUsage(char* programName) {
    cout << "Uso sugerido:" << endl;
    cout << "  Simulacion estandar: " << programName << endl;
    cout << "  Modo Benchmark:      " << programName << " -benchmark [sync_type]" << endl;
    cout << "    * sync_type: 0 para Atomic, 1 para Reduction, etc." << endl;
}

int main(int argc, char* argv[]) {
    // Parámetros físicos base
    double G = 1.0;
    double epsilon = 10.0;
    double dt = 0.01;
    unsigned int global_seed = 42; // Semilla fija para reproducibilidad

    // --- ESCENARIO 1: MODO BENCHMARK ---
    if (argc > 1 && string(argv[1]) == "-benchmark") {
        cout << ">>> Iniciando Protocolo de Benchmark <<<" << endl;
        
        // Capturamos el tipo de sincronización (por defecto 0)
        int sync_method = (argc > 2) ? stoi(argv[2]) : 0;
        
        // Configuración del experimento
        /*
        int repeticiones = 10;
        int pasos_simulacion = 500;
        int num_particulas = 1000;
        int max_hilos = 8; // Ajustar según tu CPU
        */
        int repeticiones = 5;      
        int pasos_simulacion = 500;  
        int num_particulas = 2000;
        int max_hilos = 8;
        cout << "Configuracion detectada:" << endl;
        cout << " - Particulas: " << num_particulas << endl;
        cout << " - Pasos:      " << pasos_simulacion << endl;
        cout << " - Metodo:     " << sync_method << endl;
        cout << " - Semilla:    " << global_seed << endl;

        Benchmark bm(repeticiones, pasos_simulacion, dt, global_seed);
        
        // Esta llamada ejecutara: 1. Serial Pura -> 2. Hilos OpenMP (1, 2, 4, 8...)
        bm.runScalabilityTest(max_hilos, num_particulas, sync_method);

        cout << ">>> Benchmark finalizado. Revisa los archivos .dat para el informe." << endl;
        return 0;
    }

    // --- ESCENARIO 2: MODO SIMULACIÓN ESTÁNDAR ---
    // Se ejecuta si no se pasan argumentos o si son incorrectos
    cout << ">>> Ejecutando Simulacion N-Body Estandar <<<" << endl;
    
    NBodySystem system(G, epsilon);
    
    // El setup se hace aquí para la simulación normal
    // (En el benchmark, la clase Benchmark lo hace internamente)
    mt19937 gen(global_seed);
    uniform_real_distribution<> pos_dis(0, 1000);
    uniform_real_distribution<> mass_dis(1, 100);

    int n = 500;
    for (int i = 0; i < n; ++i) {
        system.addParticle(Particle(mass_dis(gen), pos_dis(gen), pos_dis(gen)));
    }

    NBodySimulator simulator(&system, dt);
    
    // Ejecutar 1000 pasos y capturar datos
    simulation_data data = simulator.processBodies(5000);

    // Guardar resultados para análisis físico
    Visualizer vis;
    vis.exportarEnergia(data, "energy_timeseries.dat");
    vis.exportarTrayectorias(data, "trajectories.dat");
    
    cout << "Simulacion completada exitosamente." << endl;
    cout << "Archivo 'energy_timeseries.dat' generado para graficar." << endl;
    cout << "Archivo 'trajectories.dat' generado para graficar." << endl;

    return 0;
}