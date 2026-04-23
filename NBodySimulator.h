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
        void integrateEuler();
        void integrateEuler(int sync_type); // 0=atomic, 1=critical, 2=nowait
        void integrateEuler(int sync_type, bool use_barrier);
        void calculateEnergy();
        void calculateEnergy(int method); // reduce=0, atomic=1
        void calculateEnergy(int method, bool use_private);
        void processBodies();
        void processBodies(int task_type); // task=0, parallel_for=1
        void processBodies(int task_type, bool use_single);
        void simulatePhasesBarrier();
        void parallelInitializationSingle();
};

#endif