#include <iostream>
#include <cmath>
#include <gtest/gtest.h> // Incluimos la librería de GoogleTest
#include "NBodySystem.h"

// Reemplazamos "int main()" por la macro TEST de GoogleTest
TEST(NBodySystemTest, SerialVsParalelo) {
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

    // 3. Comparar con tolerancia
    double tolerancia = 1e-9; 

    for (int i = 0; i < sys_serial.getCount(); ++i) {
        double diff_x = std::abs(sys_serial.getBodies()[i].getAx() - sys_parallel.getBodies()[i].getAx());
        double diff_y = std::abs(sys_serial.getBodies()[i].getAy() - sys_parallel.getBodies()[i].getAy());

        // Usamos EXPECT_LE (Expect Less or Equal) en lugar de if/else
        // Si falla, imprimirá el mensaje personalizado
        EXPECT_LE(diff_x, tolerancia) << "Fallo en la coordenada X de la partícula " << i;
        EXPECT_LE(diff_y, tolerancia) << "Fallo en la coordenada Y de la partícula " << i;
    }
}