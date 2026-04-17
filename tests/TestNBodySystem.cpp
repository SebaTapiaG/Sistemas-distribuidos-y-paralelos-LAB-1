#include <iostream>
#include <cmath>
#include "NBodySystem.h"

int main() {
    // Sistema pequeño N=3, G=1.0, epsilon=0.1
    NBodySystem sys_serial(1.0, 0.1);
    sys_serial.addParticle(Particle(1.0, 0.0, 0.0));
    sys_serial.addParticle(Particle(2.0, 1.0, 0.5));
    sys_serial.addParticle(Particle(0.5, -1.0, -1.0));

    NBodySystem sys_parallel = sys_serial; // Copia exacta

    // 1. Ejecutar serial
    sys_serial.computeAccelerations();

    // 2. Ejecutar paralelo (ej. schedule dinámico, chunk 1)
    sys_parallel.computeAccelerations(1, 1);

    // 3. Comparar con tolerancia [cite: 524]
    double tolerancia = 1e-9; 
    bool pasoTest = true;

    for (int i = 0; i < sys_serial.getCount(); ++i) {
        double diff_x = std::abs(sys_serial.getBodies()[i].getAx() - sys_parallel.getBodies()[i].getAx());
        double diff_y = std::abs(sys_serial.getBodies()[i].getAy() - sys_parallel.getBodies()[i].getAy());

        if (diff_x > tolerancia || diff_y > tolerancia) {
            std::cerr << "Fallo en la partícula " << i << ". Dif X: " << diff_x << ", Dif Y: " << diff_y << "\n";
            pasoTest = false;
        }
    }

    if(pasoTest) std::cout << "Test Serial vs Paralelo superado con tolerancia " << tolerancia << ".\n";
    return 0;
}