#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include "NBodySimulator.h"
#include "NBodySystem.h"
#include "Particle.h"

class NBodySimulatorTest : public ::testing::Test {
protected:
    NBodySystem* system;
    NBodySimulator* simulator;
    double deltaT = 0.01; // Definimos un paso de tiempo constante para las pruebas

    void SetUp() override {
        // G = 1.0, epsilon = 0.1
        system = new NBodySystem(1.0, 0.1);
        // CORRECCIÓN: Pasamos system Y deltaT como pide tu archivo .h
        simulator = new NBodySimulator(system, deltaT);
    }

    void TearDown() override {
        delete simulator;
        delete system;
    }
};

// 1. Prueba de Energía Cinética
TEST_F(NBodySimulatorTest, KineticEnergyWithSetters) {
    Particle p(2.0, 0.0, 0.0);
    p.setVx(1.0);
    p.setVy(1.0); // v^2 = 2
    
    system->addParticle(p);
    
    // Como no tienes setG(0), para aislar K en una sola iteración, 
    // verificamos el valor inicial de k devuelto por processBodies.
    simulation_data data = simulator->processBodies(1);
    
    // K = 0.5 * m * v^2 = 0.5 * 2.0 * 2 = 2.0
    EXPECT_NEAR(data.k[0], 2.0, 1e-9);
}

// 2. Prueba de Energía Potencial
TEST_F(NBodySimulatorTest, PotentialEnergyCalculation) {
    double m1 = 1.0, m2 = 1.0;
    double dist = 1.0;
    double eps = system->getEpsilon();
    double G = system->getG();

    system->addParticle(Particle(m1, 0.0, 0.0));
    system->addParticle(Particle(m2, dist, 0.0));
    
    simulation_data data = simulator->processBodies(1);
    double expected_u = -G * (m1 * m2) / std::sqrt(dist * dist + eps * eps);
    EXPECT_NEAR(data.u[0], expected_u, 1e-7);
}

// 3. Prueba de Movimiento (Drift)
TEST_F(NBodySimulatorTest, ParticleMovesCorrectly) {
    Particle p(1.0, 0.0, 0.0);
    p.setVx(1.0); 
    system->addParticle(p);
    
    // Tras 1 iteración, con dt = 0.01, x debe ser 0.01 (Euler simple)
    simulator->processBodies(2);
    const auto& bodies = system->getBodies();
    EXPECT_NEAR(bodies[0].getX(), 0.02, 1e-7);
}