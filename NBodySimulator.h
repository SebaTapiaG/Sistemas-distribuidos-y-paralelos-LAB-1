#ifndef NBODY_SIMULATOR_H
#define NBODY_SIMULATOR_H

#include "NBodySystem.h"

using namespace std;

// Información de los cuerpos y energías para cada paso de la simulación.
struct simulation_data {
    vector<double> k;
    vector<double> u;
    vector<vector<Particle>> bodies;
};

/**
 * NBodySimulator
 * 
 * Motor encargado de coordinar la evolución temporal del sistema.
 * Utiliza sobrecarga de métodos 
 * para comparar distintas estrategias de paralelización.
 */
class NBodySimulator {
    private:
        NBodySystem* system; //Sistema de N cuerpos
        double time_step; //Paso del tiempo
    public:
        NBodySimulator(NBodySystem* sys, double dt);

        // Calcula velocidades y posiciones
        void integrateEuler();
        void integrateEuler(int sync_type); // 0=atomic, 1=critical, 2=nowait
        void integrateEuler(int sync_type, bool use_barrier);

        // Calcula y retorna energía cinética y potencial
        pair<double, double> calculateEnergy();
        pair<double, double> calculateEnergy(int method); // reduce=0, atomic=1
        pair<double, double> calculateEnergy(int method, bool use_private);

        /*
        Simula el movimiento de las partículas
        Retorna la estructura simulation_data, 
        que contiene la evolución de la energía cinética, potencial
        y el estado de las partículas en cada iteración.
        */
        simulation_data processBodies(int iter);
        simulation_data processBodies(int iter, int task_type, int sync_type, int method, int schedule_type, int chunk_size);
        simulation_data processBodies(int iter, int task_type, bool use_single, int sync_type, 
        int method, int schedule_type, int chunk_size, int use_private, int use_barrier);
        void parallelInitializationSingle();

        void simulatePhasesBarrier();

        // Métodos dedicados a demostrar cláusulas puntuales
        double calculateMetricsFirstprivate();
        double calculateFinalStateLastprivate();

        // ── Métodos de Ejecución en GPU (CUDA) ───────────────────────────────────────

        // Avanza 1 paso temporal en GPU (Aceleraciones GPU + Integración Euler GPU)
        void integrateEulerGpu(int variant = 0, int block_size = 256);

        //Calculo de fisica y energia umificado
        void stepEulerGpu(int variant = 0, int energy_method = 0, int block_size = 256, double* d_u_ptr = nullptr, double* d_k_ptr = nullptr);

        

        // Calcula energía potencial (u) y cinética (k) directamente desde buffers GPU
        void calculateEnergyGpu(int method, int block_size, double* d_u_out, double* d_k_out);

        //temporalmente reemplazada por la version con grabacion de fotogramas opcional
        /*
        // Bucle principal de simulación ejecutado en GPU
        simulation_data processBodiesGpu(int iter, int variant = 0, int energy_method = 0, int block_size = 256);
        */
        // Bucle principal de simulación ejecutado en GPU con opción de grabar fotogramas
        simulation_data processBodiesGpu(
        int iter, 
        int variant = 0, 
        int energy_method = 0, 
        int block_size = 256, 
        bool record_frames = false);
        /**
        * Resetea el estado de los buffers en la GPU cargando nuevamente 
        * los datos iniciales desde el NBodySystem de la CPU.
        * Útil para aislar benchmarks Kernel-Only sin medir transferencias en el tiempo.
        */
        void resetGpuStateFromBase();
};


#endif