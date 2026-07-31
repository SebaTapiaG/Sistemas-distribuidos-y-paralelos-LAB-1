#include <iostream>
#include <cassert>
#include <cmath>
#include "../Particle.h"
#include "../NBodySystem.h"
#include "../NBodySimulator.h"

// ── Función auxiliar para el test ───────────────────────────────────────────
// Usa el constructor existente Particle(m, x, y) y asigna velocidades con setters
Particle makeParticle(double m, double x, double y, double vx = 0.0, double vy = 0.0) {
    Particle p(m, x, y);
    p.setVx(vx);
    p.setVy(vy);
    return p;
}

// ── Test 1: Equivalencia CPU vs GPU ─────────────────────────────────────────
// ── Test 1: Equivalencia CPU vs GPU ─────────────────────────────────────────
void testCpuVsGpuEquivalence() {
    std::cout << "[TEST] Running CPU vs GPU Equivalence Test..." << std::endl;

    NBodySystem sys_cpu(1.0, 0.1);
    NBodySystem sys_gpu(1.0, 0.1);

    // Creación usando únicamente Particle(m, x, y) + setters
    sys_cpu.addParticle(makeParticle(1.0, 0.5, -0.5, 0.1, -0.1));
    sys_cpu.addParticle(makeParticle(2.0, -0.5, 0.5, -0.1, 0.1));

    sys_gpu.addParticle(makeParticle(1.0, 0.5, -0.5, 0.1, -0.1));
    sys_gpu.addParticle(makeParticle(2.0, -0.5, 0.5, -0.1, 0.1));

    NBodySimulator sim_cpu(&sys_cpu, 0.01);
    NBodySimulator sim_gpu(&sys_gpu, 0.01);

    int steps = 10;
    
    // 1. Simulación en CPU
    for (int i = 0; i < steps; ++i) {
        sim_cpu.integrateEuler();
    }

    // 2. Simulación en GPU (retorna el struct simulation_data)
    simulation_data gpu_data = sim_gpu.processBodiesGpu(steps, 0, 0, 128);

    // ── 3. VALIDACIÓN ────────────────────────────────────────────────────────
    const auto& bodies_cpu = sys_cpu.getBodies();
    // Tomamos la última iteración guardada en GPU (paso steps - 1)
    const auto& bodies_gpu = gpu_data.bodies[steps - 1]; 

    // A. Comprobar que la cantidad de cuerpos sea consistente
    assert(bodies_cpu.size() == bodies_gpu.size() && "Error: El número de partículas difiere.");

    // Tolerancia aceptable para diferencias de precisión flotante entre CPU y GPU
    const double tol = 1e-4;

    // B. Iterar sobre cada partícula y comparar coordenadas físicas
    for (size_t i = 0; i < bodies_cpu.size(); ++i) {
        double diff_x  = std::abs(bodies_cpu[i].getX()  - bodies_gpu[i].getX());
        double diff_y  = std::abs(bodies_cpu[i].getY()  - bodies_gpu[i].getY());
        double diff_vx = std::abs(bodies_cpu[i].getVx() - bodies_gpu[i].getVx());
        double diff_vy = std::abs(bodies_cpu[i].getVy() - bodies_gpu[i].getVy());

        // Aserciones de posición (X, Y)
        assert(diff_x < tol && "Divergencia detectada en la posición X entre CPU y GPU.");
        assert(diff_y < tol && "Divergencia detectada en la posición Y entre CPU y GPU.");
        
        // Aserciones de velocidad (Vx, Vy)
        assert(diff_vx < tol && "Divergencia detectada en la velocidad Vx entre CPU y GPU.");
        assert(diff_vy < tol && "Divergencia detectada en la velocidad Vy entre CPU y GPU.");
    }

    std::cout << "  ✓ Test CPU vs GPU completado y validado con exito (Tol: " << tol << ")." << std::endl;
}
// ── Test 2: Pipeline GPU ────────────────────────────────────────────────────
void testProcessBodiesGpuPipeline() {
    std::cout << "[TEST] Running GPU Pipeline Test..." << std::endl;

    NBodySystem sys(1.0, 0.1);
    sys.addParticle(makeParticle(10.0, 0.0, 0.0, 0.0, 0.0));
    sys.addParticle(makeParticle(1.0, 1.0, 0.0, 0.0, 1.0));

    NBodySimulator sim(&sys, 0.01);
    sim.processBodiesGpu(5, 0, 0, 256);

    std::cout << "  ✓ Test Pipeline GPU completado con éxito." << std::endl;
}

// ── Test 3: Variantes de Kernels ───────────────────────────────────────────
void testGpuKernelVariants() {
    std::cout << "[TEST] Running GPU Variants & Correctness Test..." << std::endl;

    const int steps = 30;
    const double dt = 0.01;
    const double tol = 1e-4; // Tolerancia flotante aceptable

    // ── 1. Inicialización de Sistemas Identicos ─────────────────────────────
    NBodySystem sys_gpu_v0(1.0, 0.1);
    NBodySystem sys_gpu_v1(1.0, 0.1);
    NBodySystem sys_cpu(1.0, 0.1);

    // Agregar partículas idénticas a cada sistema
    auto p1 = makeParticle(5.0, 1.0, 2.0, 0.1, 0.2);
    auto p2 = makeParticle(3.0, -1.0, -2.0, -0.1, -0.2);

    sys_gpu_v0.addParticle(p1); sys_gpu_v0.addParticle(p2);
    sys_gpu_v1.addParticle(p1); sys_gpu_v1.addParticle(p2);
    sys_cpu.addParticle(p1);    sys_cpu.addParticle(p2);

    // ── 2. Ejecución de Simulación ──────────────────────────────────────────
    NBodySimulator sim_gpu_v0(&sys_gpu_v0, dt);
    NBodySimulator sim_gpu_v1(&sys_gpu_v1, dt);
    NBodySimulator sim_cpu(&sys_cpu, dt);

    // Correr Variante 0 (block_size = 128) y Variante 1 (block_size = 256)
    simulation_data data_v0 = sim_gpu_v0.processBodiesGpu(steps, 0, 0, 128);
    simulation_data data_v1 = sim_gpu_v1.processBodiesGpu(steps, 1, 0, 256);

    // Correr CPU de referencia
    for (int i = 0; i < steps; ++i) {
        sim_cpu.integrateEuler();
    }
    const auto& bodies_cpu = sys_cpu.getBodies();

    // ── 3. Comprobación y Aserciones ────────────────────────────────────────

    // Validación A: Ausencia de NaNs / Infs en la energía devuelta
    for (int i = 0; i < steps; ++i) {
        assert(!std::isnan(data_v0.u[i]) && !std::isinf(data_v0.u[i]));
        assert(!std::isnan(data_v0.k[i]) && !std::isinf(data_v0.k[i]));
    }

    // Validación B: Conservación razonable de Energía Total (E = U + K)
    double E_inicial = data_v0.u[0] + data_v0.k[0];
    double E_final   = data_v0.u[steps - 1] + data_v0.k[steps - 1];
    double delta_E   = std::abs(E_final - E_inicial) / std::abs(E_inicial);
    assert(delta_E < 0.05); // La energía no debe variar más de un 5% en 30 pasos

    // Validación C: Coincidencia entre CPU vs GPU (Variante 0) y Variante 0 vs Variante 1
    const auto& bodies_gpu_v0 = data_v0.bodies[steps - 1];
    const auto& bodies_gpu_v1 = data_v1.bodies[steps - 1];

    for (size_t i = 0; i < bodies_cpu.size(); ++i) {
        // Coincidencia CPU vs GPU V0
        assert(std::abs(bodies_cpu[i].getX()  - bodies_gpu_v0[i].getX())  < tol);
        assert(std::abs(bodies_cpu[i].getY()  - bodies_gpu_v0[i].getY())  < tol);
        assert(std::abs(bodies_cpu[i].getVx() - bodies_gpu_v0[i].getVx()) < tol);
        assert(std::abs(bodies_cpu[i].getVy() - bodies_gpu_v0[i].getVy()) < tol);

        // Coincidencia GPU Variante 0 vs GPU Variante 1
        assert(std::abs(bodies_gpu_v0[i].getX() - bodies_gpu_v1[i].getX()) < tol);
        assert(std::abs(bodies_gpu_v0[i].getY() - bodies_gpu_v1[i].getY()) < tol);
    }

    std::cout << "  ✓ Test de variantes completado y validado correctamente." << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "      EJECUTANDO SUITE DE TEST GPU      " << std::endl;
    std::cout << "========================================" << std::endl;

    testCpuVsGpuEquivalence();
    testProcessBodiesGpuPipeline();
    testGpuKernelVariants();

    std::cout << "\n¡TODOS LOS TESTS PASARON CORRECTAMENTE!" << std::endl;
    return 0;
}