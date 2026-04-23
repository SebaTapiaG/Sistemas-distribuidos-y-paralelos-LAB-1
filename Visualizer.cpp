#include "Visualizer.h"
#include <fstream>
#include <iostream>

using namespace std;

Visualizer::Visualizer() {
    archivo.open("Snapshot.dat");
    if (!archivo.is_open()) {
        cerr << "Error al abrir el archivo para visualización." << endl;
    }
    // Constructor vacío o inicialización de recursos gráficos si es necesario
}

void Visualizer::abrirArchivo() {
    if (!archivo.is_open()) {
        archivo.open("Snapshot.dat");
    }
    cout << "Visualizer: Archivo abierto" << endl;
}

void Visualizer::capturarEstado(const NBodySystem& system) {

    cout << "Visualizer iniciado" << endl;

    const auto& bodies = system.getBodies();

    for (size_t i = 0; i < bodies.size(); ++i) {
        const Particle& p = bodies[i];
        archivo << i << " " << p.getMass() << " " << p.getX() << " " << p.getY() << " " << p.getVx() << " " << p.getVy() << " " << p.getAx() << " " << p.getAy() << std::endl;
    }

    cout << "Estado capturado por Visualizer" << endl;
}

void Visualizer::cerrarArchivo() {
    if (archivo.is_open()) {
        archivo.close();
    }
    cout << "Visualizer: Archivo cerrado" << endl;

}
