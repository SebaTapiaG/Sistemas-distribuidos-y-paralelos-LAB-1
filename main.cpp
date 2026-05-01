#include <iostream>
#include "Particle.h"
#include "NBodySimulator.h"
#include <random>
#include "Visualizer.h"

using namespace std;


int main(){
    cout << "Flag Inicial" << endl;
    
    // Definir G
    double g = 1.0;
    // Definir epsilon
    double epsilon = 10;
    //Definir delta t
    double delta_t = 0.01; // Paso temporal

    cout << "Flag Definiciones" << endl;

    // Crear sistema NBodySystem con G y epsilon
    NBodySystem system(g, epsilon);
    // Agregar partículas al sistema (ejemplo: 3 cuerpos con masas y posiciones iniciales)

    cout << "Flag NBodySystem" << endl;

    int cantidad_particulas = 500;

    // Generador aleatorio con seed fija para resultados reproducibles
    unsigned int seed = 123;
    mt19937 gen(seed);

    // Generar numero de posición aleatoria
    uniform_real_distribution<> random_dis(0, 1000);

    // Generar numero de masa aleatoria
    uniform_real_distribution<> random_mass(1, 100);

    cout << "Flag Random numbers" << endl;

    for (int i = 0; i < cantidad_particulas; ++i) {
        double mass = random_mass(gen);
        double x = random_dis(gen);
        double y = random_dis(gen);
        Particle p(mass, x, y);
        system.addParticle(p);
        
    }

    cout << "Flag Particulas" << endl;

    // Crear simulador NBodySimulator con el sistema
    NBodySimulator simulator(&system, delta_t);

    cout << "Flag NBodySimulator" << endl;
    
    // Llamar a simulate() para ejecutar la simulación

    int num_steps = 5000;   // Numero de pasos de la simulacion
    cout << "Iniciando simulacion de " << num_steps << " pasos..." << endl;
    
    // 1. Correr la simulación y guardar el historial de energía
    simulation_data data = simulator.processBodies(num_steps);
    cout << "Flag Simulacion terminada" << endl;

    // 2. Exportar los datos usando tu Visualizer
    Visualizer vis;
    vis.exportarEnergia(data, "energy_timeseries.dat");
    
    // 3. Exportar las posiciones finales (Snapshot)
    vis.abrirArchivo();
    vis.capturarEstado(system);
    vis.cerrarArchivo();

    return 0;

}