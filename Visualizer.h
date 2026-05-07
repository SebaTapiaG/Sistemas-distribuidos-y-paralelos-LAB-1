#ifndef VISUALIZER_H
#define VISUALIZER_H
#include "NBodySystem.h"
#include "NBodySimulator.h"
#include <fstream>

using namespace std;

class Visualizer {
    public:
        ofstream archivo;

        Visualizer();

        void abrirArchivo();
        
        /**
        * Captura el estado actual del sistema NBodySystem para visualización.
        * Este método se llamará en cada paso de la simulación para actualizar la visualización.
        * @param system Referencia constante al sistema NBodySystem a visualizar.
        * Nota: Las columnas de salida son: id_particula, masa, x, y, vx, vy, ax, ay
        */
        void capturarEstado(const NBodySystem& system);

        void cerrarArchivo();

        void exportarEnergia(const simulation_data& data, const std::string& filename);

        void exportarTrayectorias(const simulation_data& data, const std::string& filename);
};

#endif