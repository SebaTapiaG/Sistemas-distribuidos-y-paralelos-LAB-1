#ifndef NBODY_SIMULATOR_H
#define NBODY_SIMULATOR_H

#include "NBodySystem.h"

using namespace std;

struct simulation_data {
    vector<double> k;
    vector<double> u;
    vector<vector<Particle>> bodies;
};

class NBodySimulator {
    private:
        NBodySystem* system;
        double time_step;
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
        simulation_data processBodies(int iter, int task_type); // task=0, parallel_for=1
        simulation_data processBodies(int iter, int task_type, int sync_type, int method, int schedule_type, int chunk_size);
        void simulatePhasesBarrier();
        void parallelInitializationSingle();

        // Métodos dedicados a demostrar cláusulas puntuales
        double calculateMetricsFirstprivate();
        double calculateFinalStateLastprivate();
};

#endif