#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include "../NBodySimulator.h"
#include "../NBodySystem.h"
#include "../Particle.h"

class NBodySimulatorTest : public ::testing::Test {
protected:
    NBodySystem*    system;
    NBodySimulator* simulator;
    const double    deltaT = 0.01;

    void SetUp() override {
        system    = new NBodySystem(1.0, 0.1);
        simulator = new NBodySimulator(system, deltaT);
    }
    void TearDown() override { delete simulator; delete system; }
};

// ── 1. Energía cinética inicial ──────────────────────────────────────────────
TEST_F(NBodySimulatorTest, KineticEnergyWithSetters) {
    Particle p(2.0, 0.0, 0.0);
    p.setVx(1.0); p.setVy(1.0);   // |v|^2 = 2
    system->addParticle(p);
    // K = 0.5 * 2.0 * 2 = 2.0  ANTES de integrar
    // processBodies(1) integra primero y luego mide => K cambia.
    // Verificar K inicial con calculateEnergy() directamente.
    auto [u0, k0] = simulator->calculateEnergy(0);
    EXPECT_NEAR(k0, 2.0, 1e-9);
}

// ── 2. Energía potencial inicial ─────────────────────────────────────────────
// CORRECCIÓN RESPECTO AL TEST ORIGINAL:
// processBodies(1) hace: computeAccelerations -> integrateEuler -> calculateEnergy
// Por tanto data.u[0] es U DESPUÉS del primer paso Euler, no en t=0.
// La U inicial se debe medir con calculateEnergy() antes de llamar processBodies.
TEST_F(NBodySimulatorTest, PotentialEnergyInitialState) {
    const double m1 = 1.0, m2 = 1.0, dist = 1.0;
    const double eps = system->getEpsilon();
    const double G   = system->getG();

    system->addParticle(Particle(m1, 0.0,  0.0));
    system->addParticle(Particle(m2, dist, 0.0));

    // Medir U en t=0, ANTES de integrar
    auto [u0, k0] = simulator->calculateEnergy(0);
    const double expected_u0 = -G * m1 * m2 / std::sqrt(dist*dist + eps*eps);
    EXPECT_NEAR(u0, expected_u0, 1e-9);
}

// ── 3. Energía potencial después de un paso (lo que realmente hace processBodies) ──
TEST_F(NBodySimulatorTest, PotentialEnergyAfterOneStep) {
    // El orden en processBodies(iter) del simulador es:
    //   1. computeAccelerations()
    //   2. calculateEnergy()   <-- guarda U aqui, en el estado ACTUAL (t=0)
    //   3. integrateEuler()    <-- mueve las particulas DESPUES
    // Por tanto data.u[0] corresponde a U en t=0, igual que PotentialEnergyInitialState.
    // Este test verifica que U en t=0 coincide con el valor analitico esperado.
    const double m1 = 1.0, m2 = 1.0, dist = 1.0;
    const double eps = system->getEpsilon();
    const double G   = system->getG();

    system->addParticle(Particle(m1, 0.0,  0.0));
    system->addParticle(Particle(m2, dist, 0.0));

    // U en t=0: -G*m1*m2 / sqrt(d^2 + eps^2)
    const double expected_u = -G * m1 * m2 / std::sqrt(dist*dist + eps*eps);

    simulation_data data = simulator->processBodies(1);
    EXPECT_NEAR(data.u[0], expected_u, 1e-9);
}

// ── 4. Movimiento con velocidad inicial ──────────────────────────────────────
// Una partícula aislada (sin otras que la atraigan) con vx=1
// debe avanzar x += vx*dt en cada paso.
// Con 2 pasos: x = 0 + 1*0.01 + 1*0.01 ≈ 0.02 (la aceleracion gravitatoria
// es cero con una sola particula, por lo que vx no cambia).
TEST_F(NBodySimulatorTest, ParticleMovesCorrectly) {
    Particle p(1.0, 0.0, 0.0);
    p.setVx(1.0);
    system->addParticle(p);
    simulator->processBodies(2);
    EXPECT_NEAR(system->getBodies()[0].getX(), 0.02, 1e-7);
}

// ── 5. Historial y estabilidad en múltiples iteraciones ──────────────────────
TEST_F(NBodySimulatorTest, MultipleIterationsIntegration) {
    system->addParticle(Particle(1.0, 0.0, 0.0));
    system->addParticle(Particle(1.0, 5.0, 0.0));
    const int iters = 10;
    simulation_data data = simulator->processBodies(iters);

    EXPECT_EQ((int)data.bodies.size(), iters);
    EXPECT_EQ((int)data.u.size(),      iters);
    EXPECT_EQ((int)data.k.size(),      iters);

    // Sin NaN
    EXPECT_FALSE(std::isnan(data.u[iters-1]));
    EXPECT_FALSE(std::isnan(data.k[iters-1]));

    // La partícula 2 debe haberse acercado al origen (atracción gravitatoria)
    EXPECT_LT(data.bodies[iters-1][1].getX(), 5.0);
}

// ── 6. Métodos paralelos de energía (reduction vs atomic) ────────────────────
TEST_F(NBodySimulatorTest, ParallelEnergyMethodsMatch) {
    system->addParticle(Particle(2.0, 0.0, 0.0));
    Particle p2(1.5, 3.0, 4.0);
    p2.setVx(1.0); p2.setVy(-2.0);
    system->addParticle(p2);

    auto [u_red,  k_red]  = simulator->calculateEnergy(0);   // reduction
    auto [u_atom, k_atom] = simulator->calculateEnergy(1);   // atomic

    EXPECT_NEAR(u_red, u_atom, 1e-9);
    EXPECT_NEAR(k_red, k_atom, 1e-9);
}

// ── 7. integrateEuler con distintos sync_type ─────────────────────────────────
TEST_F(NBodySimulatorTest, IntegrationSyncTypesDoNotCrash) {
    system->addParticle(Particle(1.0, 0.0, 0.0));
    system->addParticle(Particle(1.0, 2.0, 2.0));
    system->computeAccelerations();

    const double x_inicial = system->getBodies()[1].getX();

    ASSERT_NO_THROW(simulator->integrateEuler(1));   // critical
    const double x_critical = system->getBodies()[1].getX();
    EXPECT_NE(x_critical, x_inicial);

    ASSERT_NO_THROW(simulator->integrateEuler(2));   // nowait
    const double x_nowait = system->getBodies()[1].getX();
    EXPECT_NE(x_nowait, x_critical);
}

// ── 8. processBodies con task_type 0 y 1 ─────────────────────────────────────
TEST_F(NBodySimulatorTest, ProcessBodiesTaskTypes) {
    system->addParticle(Particle(1.0, 0.0, 0.0));
    system->addParticle(Particle(1.0, 1.0, 0.0));

    ASSERT_NO_THROW({
        simulation_data d = simulator->processBodies(3, 0, 1, 0, 0, 100);  // tasks
        EXPECT_EQ((int)d.bodies.size(), 3);
    });
    ASSERT_NO_THROW({
        simulation_data d = simulator->processBodies(3, 1, 1, 0, 0, 100);  // parallel_for
        EXPECT_EQ((int)d.bodies.size(), 3);
    });
}

// ── 9. parallelInitializationSingle ──────────────────────────────────────────
TEST_F(NBodySimulatorTest, ParallelInitializationSingle) {
    ASSERT_NO_THROW(simulator->parallelInitializationSingle());
    EXPECT_DOUBLE_EQ(system->getG(),       1.0);
    EXPECT_DOUBLE_EQ(system->getEpsilon(), 0.1);
}

// ── 10. simulatePhasesBarrier no produce NaN ──────────────────────────────────
TEST_F(NBodySimulatorTest, SimulatePhasesBarrierIsCorrect) {
    system->addParticle(Particle(1.0, 0.0, 0.0));
    system->addParticle(Particle(1.0, 1.0, 0.0));
    ASSERT_NO_THROW(simulator->simulatePhasesBarrier());
    for (const auto& b : system->getBodies()) {
        EXPECT_FALSE(std::isnan(b.getX()));
        EXPECT_FALSE(std::isnan(b.getY()));
        EXPECT_FALSE(std::isnan(b.getVx()));
        EXPECT_FALSE(std::isnan(b.getVy()));
    }
}

// ── 11. Resultados task vs parallel_for son físicamente equivalentes ──────────
// Mismo sistema, misma semilla => ambos métodos deben dar el mismo estado final.
TEST_F(NBodySimulatorTest, TaskAndParallelForGiveSameResult) {
    const int iters = 5;

    // 1. Probamos con parallel_for (task_type = 1)
    NBodySystem  sysB(1.0, 0.1);
    sysB.addParticle(Particle(1.0, 0.0, 0.0));
    sysB.addParticle(Particle(2.0, 1.0, 0.5));
    NBodySimulator simB(&sysB, deltaT);
    simB.processBodies(iters, 1, 1, 0, 0, 100);

    EXPECT_NE(sysB.getBodies()[0].getX(), 0.0) << "parallel_for debe mover particulas";

    // 2. Probamos con tasks (task_type = 0)
    NBodySystem  sysA(1.0, 0.1);
    sysA.addParticle(Particle(1.0, 0.0, 0.0));
    sysA.addParticle(Particle(2.0, 1.0, 0.5));
    NBodySimulator simA(&sysA, deltaT);
    
    // Ya no usamos ASSERT_NO_THROW para ocultar el error, ahora exigimos que funcione
    simA.processBodies(iters, 0, 1, 0, 0, 100);
    EXPECT_NE(sysA.getBodies()[0].getX(), 0.0) << "tasks ahora debe mover particulas";

    // 3. Verificamos que ambos métodos de paralelización producen la misma física
    EXPECT_NEAR(sysA.getBodies()[0].getX(), sysB.getBodies()[0].getX(), 1e-7);
    EXPECT_NEAR(sysA.getBodies()[0].getY(), sysB.getBodies()[0].getY(), 1e-7);
    EXPECT_NEAR(sysA.getBodies()[1].getX(), sysB.getBodies()[1].getX(), 1e-7);
    EXPECT_NEAR(sysA.getBodies()[1].getY(), sysB.getBodies()[1].getY(), 1e-7);
}

// ── 12. Conservación aproximada del momento lineal ───────────────────────────
TEST_F(NBodySimulatorTest, LinearMomentumIsConserved) {
    // Con solo fuerzas internas, Σ m_i * v_i debe mantenerse constante
    system->addParticle(Particle(1.0,  0.0, 0.0));
    system->addParticle(Particle(1.0,  2.0, 0.0));
    system->addParticle(Particle(2.0, -1.0, 1.5));

    // Momento inicial (velocidades = 0 => P0 = 0)
    double px0 = 0.0, py0 = 0.0;

    simulator->processBodies(20);

    double px_final = 0.0, py_final = 0.0;
    for (const auto& b : system->getBodies()) {
        px_final += b.getMass() * b.getVx();
        py_final += b.getMass() * b.getVy();
    }
    // Euler no conserva energía exactamente pero sí momento (fuerzas internas)
    EXPECT_NEAR(px_final, px0, 1e-9);
    EXPECT_NEAR(py_final, py0, 1e-9);
}