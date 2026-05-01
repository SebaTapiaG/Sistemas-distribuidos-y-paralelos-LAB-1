#include "NBodySystem.h"
#include <omp.h>
#include <cmath>
#include <stdexcept>

// ── Constructor ──────────────────────────────────────────────────────────────

NBodySystem::NBodySystem(double G, double epsilon)
    : G_const(G), softening_eps(epsilon) {
    if (epsilon <= 0.0)
        throw std::invalid_argument("epsilon debe ser > 0 para evitar singularidades");
    if (G < 0.0)
        throw std::invalid_argument("G debe ser >= 0");
}

// ── Gestión de partículas ────────────────────────────────────────────────────

void NBodySystem::addParticle(const Particle& p) {
    if (p.getMass() <= 0.0)
        throw std::invalid_argument("La masa de cada partícula debe ser > 0");
    bodies.push_back(p);
}

void NBodySystem::zeroAccelerations() {
    for (auto& b : bodies)
        b.zeroAcceleration();
}


// ── Cálculo de aceleraciones (serial) ───────────────────────────────────────

void NBodySystem::computeAccelerations() {
    const int N = static_cast<int>(bodies.size());
    const double eps2 = softening_eps * softening_eps;  // eps^2, precalculado

    zeroAccelerations();

    /*
     * Bucle par a par — convención del enunciado:
     *   - Bucle externo sobre i  (cuerpo receptor de la fuerza)
     *   - Bucle interno sobre j != i (cuerpo fuente)
     *   - Solo se escribe en bodies[i].ax/ay; bodies[j] solo se lee.
     *
     * Esto evita condiciones de carrera cuando el bucle externo
     * se paralelice con OpenMP en semanas posteriores.
     *
     * Fórmula aplicada para cada par (i, j):
     *
     *   dx  = xj - xi
     *   dy  = yj - yi
     *   s²  = dx² + dy² + eps²          (distancia suavizada al cuadrado)
     *   s³  = s² ^ (3/2)
     *   factor = G * mj / s³
     *   ax_i += factor * dx
     *   ay_i += factor * dy
     */
    for (int i = 0; i < N; ++i) {
        double acc_x = 0.0;
        double acc_y = 0.0;

        const double xi = bodies[i].getX();
        const double yi = bodies[i].getY();

        for (int j = 0; j < N; ++j) {
            if (j == i) continue;

            const double dx = bodies[j].getX() - xi;
            const double dy = bodies[j].getY() - yi;

            // Distancia suavizada al cuadrado: |rj - ri|^2 + eps^2
            const double dist2 = dx * dx + dy * dy + eps2;

            // s^3 = (dist2)^(3/2)
            const double dist3 = dist2 * std::sqrt(dist2);

            // Contribución de j sobre i: G * mj / s^3
            const double factor = G_const * bodies[j].getMass() / dist3;

            acc_x += factor * dx;
            acc_y += factor * dy;
        }

        bodies[i].setAcceleration(acc_x, acc_y);
    }
}

// ── Variantes con OpenMP (stubs para semana 2) ───────────────────────────────

void NBodySystem::computeAccelerations(int schedule_type, int chunk_size) {
    const int N = static_cast<int>(bodies.size());
    const double eps2 = softening_eps * softening_eps;

    zeroAccelerations();

    // Configurar el schedule dinámicamente según el parámetro [cite: 761, 762]
    omp_sched_t kind;
    if (schedule_type == 0) kind = omp_sched_static;
    else if (schedule_type == 1) kind = omp_sched_dynamic;
    else if (schedule_type == 2) kind = omp_sched_guided;
    else kind = omp_sched_auto;

    omp_set_schedule(kind, chunk_size > 0 ? chunk_size : 0);

    // Paralelización del bucle externo [cite: 498]
    #pragma omp parallel for schedule(runtime)
    for (int i = 0; i < N; ++i) {
        double acc_x = 0.0;
        double acc_y = 0.0;
        const double xi = bodies[i].getX();
        const double yi = bodies[i].getY();

        for (int j = 0; j < N; ++j) {
            if (j == i) continue;
            const double dx = bodies[j].getX() - xi;
            const double dy = bodies[j].getY() - yi;
            const double dist2 = dx * dx + dy * dy + eps2;
            const double dist3 = dist2 * std::sqrt(dist2);
            const double factor = G_const * bodies[j].getMass() / dist3;

            acc_x += factor * dx;
            acc_y += factor * dy;
        }
        bodies[i].setAcceleration(acc_x, acc_y);
    }
}

// Sobrecarga sin chunk size explícito
void NBodySystem::computeAccelerations(int schedule_type) {
    computeAccelerations(schedule_type, 0); 
}

void NBodySystem::computeAccelerationsCollapse() {
    const int N = static_cast<int>(bodies.size());
    const double eps2 = softening_eps * softening_eps;

    zeroAccelerations();

    /* * Al usar collapse(2), el espacio de iteraciones es (i, j) plano.
     * Múltiples hilos pueden procesar pares con el mismo 'i' al mismo tiempo.
     * Por ello, usamos addAcceleration() que internamente tiene #pragma omp atomic[cite: 453, 500].
     */
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == j) continue; // No se puede usar 'continue' directamente fuera de un if simple en algunos compiladores, pero aquí es válido.

            const double dx = bodies[j].getX() - bodies[i].getX();
            const double dy = bodies[j].getY() - bodies[i].getY();
            const double dist2 = dx * dx + dy * dy + eps2;
            const double dist3 = dist2 * std::sqrt(dist2);
            const double factor = G_const * bodies[j].getMass() / dist3;

            // Se suma atómicamente a las componentes de aceleración de la partícula i
            bodies[i].addAcceleration(factor * dx, factor * dy);
        }
    }
}

// ── Acceso al estado ─────────────────────────────────────────────────────────

const std::vector<Particle>& NBodySystem::getBodies() const { return bodies; }
std::vector<Particle>& NBodySystem::getBodies() { return bodies; }

double NBodySystem::getG()       const { return G_const; }
double NBodySystem::getEpsilon() const { return softening_eps; }

void NBodySystem::setG(double newG) {
    G_const = newG;
}

void NBodySystem::setEpsilon(double newEps) {
    softening_eps = newEps;
}