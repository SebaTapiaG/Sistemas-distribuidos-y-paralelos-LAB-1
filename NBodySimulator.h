#ifndef NBODY_SIMULATOR_H
#define NBODY_SIMULATOR_H

#include "NBodySystem.h"

using namespace std;

class NBodySimulator {
    private:
        NBodySystem* system;
        double g_val;
        double epsilon_val;

    public:
        NBodySimulator(NBodySystem* sys, double G, double epsilon);
        void simulate(double delta_t, int num_steps);
        NBodySystem* getSystem() const;
        double getG() const;
        double getEpsilon() const;
};

#endif