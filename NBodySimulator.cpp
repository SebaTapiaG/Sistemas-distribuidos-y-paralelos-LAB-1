#include "NBodySimulator.h"
#include "Visualizer.h"
#include <iostream>

NBodySimulator::NBodySimulator(NBodySystem* sys, double G, double epsilon) {
    system = sys;
    g_val = G;
    epsilon_val = epsilon;
}

NBodySystem* NBodySimulator::getSystem() const {
    return system;
}

double NBodySimulator::getG() const {
    return g_val;
}

double NBodySimulator::getEpsilon() const {
    return epsilon_val;
}

void NBodySimulator::simulate(double delta_t, int num_steps) {
    cout << "Flag Simulación iniciada" << endl;

    Visualizer visualizer;

    cout << "Flag Visualizer creado" << endl;


    // loop principal
    for (int step = 0; step < num_steps; step++) {
        
        // 1. calcular aceleraciones
        system->computeAccelerations(); // calcular aceleraciones

        cout << "Loop Aceleraciones" << endl;

        // 2. integrar posiciones y velocidades (e.g. Euler)
        system->computeSpeedAndPosition(delta_t); // actualizar velocidades y posiciones

        cout << "Loop Velocidad y Posición" << endl;

        // 3. calcular energía total (opcional, para monitoreo)

        // 4. (opcional) imprimir estado actual o guardar en archivo
        // Llamar a visualizer
        visualizer.capturarEstado(*system);

        cout << "Loop Visualizer terminado" << endl;
    }
}