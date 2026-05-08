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
    cout << "Uso modo Benchmark:" << endl;
    cout << "  " << programName << " -benchmark [task_type] [sync_type] [energy_method] [sched] [chunk]" << endl;
    cout << endl << "  Parametros:" << endl;
    cout << "  task_type:     0: Tasks, 1: Parallel For" << endl;
    cout << "  sync_type:     0: Atomic, 1: Critical, 2: Nowait" << endl;
    cout << "  energy_method: 0: Reduction, 1: Atomic" << endl;
    cout << "  sched:         0: Static, 1: Dynamic, 2: Guided" << endl;
    cout << "  chunk:         Tamano del chunk (ej: 1, 10, 100)" << endl;
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
        /*
        Entradas aceptadas:
        - task_type: 0 para Tasks, 1 para Parallel For
        - sync_type: 0 para Atomic, 1 para Critical, 2 para Nowait
        - energy_method: 0 para Reduction, 1 para Atomic
        - sched: 0 para Static, 1 para Dynamic, 2 para Guided
        - chunk: tamaño del chunk para el scheduler (ej: 1, 10, 100)
        */
        int task_type     = (argc > 2) ? stoi(argv[2]) : 1; // Default: Parallel For
        int sync_method   = (argc > 3) ? stoi(argv[3]) : 0; // Default: Atomic
        int energy_method = (argc > 4) ? stoi(argv[4]) : 0; // Default: Reduction
        int sched_type    = (argc > 5) ? stoi(argv[5]) : 0; // Default: Static
        int chunk_size    = (argc > 6) ? stoi(argv[6]) : 0; // Default: 0 (tamaño automático según el scheduler)
        
        // Configuración del experimento
        /*
        int repeticiones = 10;
        int pasos_simulacion = 500;
        int num_particulas = 1000;
        int max_hilos = 8; // Ajustar según tu CPU
        */
        int repeticiones = 5;      
        int pasos_simulacion = 500;  
        int num_particulas = 1000;
        int max_hilos = 8;
        cout << "Configuracion detectada:" << endl;
        cout << " - Particulas: " << num_particulas << endl;
        cout << " - Pasos:      " << pasos_simulacion << endl;
        cout << " - Metodo:     " << sync_method << endl;
        cout << " - Semilla:    " << global_seed << endl;

        Benchmark bm(repeticiones, pasos_simulacion, dt, global_seed);
        
        bm.runScalabilityTest(
            max_hilos, 
            num_particulas, 
            task_type, 
            sync_method, 
            energy_method, 
            sched_type, 
            chunk_size, 
            G, 
            epsilon
        );

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