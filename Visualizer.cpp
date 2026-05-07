#include "Visualizer.h"
#include <fstream>
#include <iostream>
#include <iomanip>

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
    const auto& bodies = system.getBodies();
    
    // NUEVO: Configuramos el archivo para máxima precisión
    archivo << std::scientific << std::setprecision(15);
    
    for (size_t i = 0; i < bodies.size(); ++i) {
        const Particle& p = bodies[i];
        archivo << i << " " 
                << p.getMass() << " " 
                << p.getX() << " " 
                << p.getY() << " " 
                << p.getVx() << " " 
                << p.getVy() << " " 
                << p.getAx() << " " 
                << p.getAy() << std::endl;
    }
}

void Visualizer::cerrarArchivo() {
    if (archivo.is_open()) {
        archivo.close();
    }
    cout << "Visualizer: Archivo cerrado" << endl;

}

void Visualizer::exportarEnergia(const simulation_data& data, const std::string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error al crear " << filename << endl;
        return;
    }
    file << "Paso K U E_Total\n"; // Cabeceras
    for (size_t i = 0; i < data.k.size(); ++i) {
        double e_total = data.k[i] + data.u[i];
        file << i << " " << data.k[i] << " " << data.u[i] << " " << e_total << "\n";
    }
    file.close();
    cout << "Datos de energia exportados a " << filename << endl;
}

void Visualizer::exportarTrayectorias(const simulation_data& data, const std::string& filename) {
    std::ofstream f(filename);
    if (!f.is_open()) return;

    f << "# Paso ID Masa X Y Vx Vy\n"; // Añadimos ID
    f << std::fixed << std::setprecision(8);

    for (size_t t = 0; t < data.bodies.size(); ++t) {
        for (size_t i = 0; i < data.bodies[t].size(); ++i) {
            const auto& p = data.bodies[t][i];
            // i actúa como el ID único de la partícula
            f << t << " " << i << " " << p.getMass() << " " << p.getX() << " " << p.getY() 
              << " " << p.getVx() << " " << p.getVy() << "\n";
        }
        f << "\n"; 
    }
    f.close();
}