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

// 4. Prueba de lazo temporal continuo (Integración)
TEST_F(NBodySimulatorTest, MultipleIterationsIntegration) {
    // Sistema básico que debería atraerse
    system->addParticle(Particle(1.0, 0.0, 0.0));
    system->addParticle(Particle(1.0, 5.0, 0.0));
    
    int iteraciones = 10;
    simulation_data data = simulator->processBodies(iteraciones);
    
    // Verificar que guardó el historial correctamente
    EXPECT_EQ(data.bodies.size(), iteraciones);
    EXPECT_EQ(data.u.size(), iteraciones);
    EXPECT_EQ(data.k.size(), iteraciones);

    // Verificar que la energía no se corrompe (NaN)
    EXPECT_FALSE(std::isnan(data.u[iteraciones - 1]));
    EXPECT_FALSE(std::isnan(data.k[iteraciones - 1]));
    
    // Verificar que la partícula 2 se movió hacia el origen (se atraen)
    double pos_final_x = data.bodies[iteraciones - 1][1].getX();
    EXPECT_LT(pos_final_x, 5.0); // LT = Less Than (menor que)
}

// 5. Prueba de Cálculo de Energía Paralela (Reduction vs Atomic)
TEST_F(NBodySimulatorTest, ParallelEnergyMethodsMatch) {
    system->addParticle(Particle(2.0, 0.0, 0.0));
    Particle p2(1.5, 3.0, 4.0);
    p2.setVx(1.0);
    p2.setVy(-2.0);
    system->addParticle(p2);

    // Calculamos con reduction (method = 0)
    auto [u_red, k_red] = simulator->calculateEnergy(0);
    
    // Calculamos con atomic (method = 1)
    auto [u_atom, k_atom] = simulator->calculateEnergy(1);

    // Ambos métodos deben dar exactamente el mismo resultado
    EXPECT_NEAR(u_red, u_atom, 1e-9);
    EXPECT_NEAR(k_red, k_atom, 1e-9);
}

// 6. Prueba de Paralelización de Integración (Sync Types)
TEST_F(NBodySimulatorTest, IntegrationSyncTypesDoNotCrash) {
    system->addParticle(Particle(1.0, 0.0, 0.0));
    system->addParticle(Particle(1.0, 2.0, 2.0));
    system->computeAccelerations(); // Para que tengan aceleración que integrar

    // Guardamos estado
    double x_inicial = system->getBodies()[1].getX();

    // Probamos integrar con critical (1)
    ASSERT_NO_THROW(simulator->integrateEuler(1));
    double x_critical = system->getBodies()[1].getX();
    EXPECT_NE(x_critical, x_inicial); // NE = Not Equal (debió moverse)

    // Probamos integrar con nowait (2)
    ASSERT_NO_THROW(simulator->integrateEuler(2));
    double x_nowait = system->getBodies()[1].getX();
    EXPECT_NE(x_nowait, x_critical); // Debió moverse aún más
}

// 7. Prueba de Tipos de Tareas (processBodies paralelo)
TEST_F(NBodySimulatorTest, ProcessBodiesTaskTypes) {
    system->addParticle(Particle(1.0, 0.0, 0.0));
    system->addParticle(Particle(1.0, 1.0, 0.0));

    // Ejecutamos con task explícito (0)
    ASSERT_NO_THROW({
        simulation_data data_task = simulator->processBodies(3, 0);
        EXPECT_EQ(data_task.bodies.size(), 3);
    });

    // Ejecutamos con parallel for (1)
    ASSERT_NO_THROW({
        simulation_data data_for = simulator->processBodies(3, 1);
        EXPECT_EQ(data_for.bodies.size(), 3);
    });
}

// 8. Prueba del bloque Single de OpenMP
TEST_F(NBodySimulatorTest, ParallelInitializationSingle) {
    // Ahora que tienes los setters implementados, esta función no debería arrojar errores.
    // Además verificamos que los valores sigan siendo los correctos (1.0 y 0.1).
    ASSERT_NO_THROW(simulator->parallelInitializationSingle());
    EXPECT_DOUBLE_EQ(system->getG(), 1.0);
    EXPECT_DOUBLE_EQ(system->getEpsilon(), 0.1);
}