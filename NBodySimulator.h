#ifndef NBODY_SIMULATOR_H
#define NBODY_SIMULATOR_H

#include "NBodySystem.h"

using namespace std;

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

        // Simula el movimiento de las partículas
        // Retorna las energías cinéticas y potenciales de cada iteración
        pair<vector<double>, vector<double>> processBodies(int iter);
        pair<vector<double>, vector<double>> processBodies(int iter, int task_type); // task=0, parallel_for=1
        pair<vector<double>, vector<double>> processBodies(int iter, int task_type, bool use_single);
        void simulatePhasesBarrier();
        void parallelInitializationSingle();
};

#endif