#include "Particle.h"
#include <iostream>
#include <random>

using namespace std;

int runTestParticle() {
    mt19937 rng(12345); // semilla fija
    uniform_real_distribution<double> dist(0.0, 100.0);

    double mass, x, y;
    mass = dist(rng);
    x = dist(rng);
    y = dist(rng);

    Particle* p = new Particle(mass, x, y);

    cout << "\nTEST PARTICLE" << endl;
    cout << "Particula creada: " << endl;
    cout << "Masa: " << p->getMass() << endl;
    cout << "Posicion: (" << p->getX() << ", " << p->getY() << ")" << endl;
    cout << "Velocidad: (" << p->getVx() << ", " << p->getVy() << ")" << endl;
    cout << "Aceleracion: (" << p->getAx() << ", " << p->getAy() << ")" << endl;

    delete p;
    return 0;
}