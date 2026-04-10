#include "NBodySystem.h"
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

void NBodySystem::computeAccelerations(int schedule_type) {
    // TODO semana 2: activar #pragma omp parallel for schedule(...)
    // según schedule_type: 0=static, 1=dynamic, 2=guided
    // Por ahora delega en la versión serial de referencia.
    (void)schedule_type;
    computeAccelerations();
}

void NBodySystem::computeAccelerations(int schedule_type, int chunk_size) {
    // TODO semana 2: igual que arriba pero con chunk_size explícito
    (void)schedule_type;
    (void)chunk_size;
    computeAccelerations();
}

void NBodySystem::computeAccelerationsCollapse() {
    // TODO semana 2: collapse(2) sobre bucles i,j con índice lineal k
    // Requiere demostrar equivalencia con la ecuación de a_i en el reporte.
    computeAccelerations();
}

// ── Acceso al estado ─────────────────────────────────────────────────────────

const std::vector<Particle>& NBodySystem::getBodies() const { return bodies; }
std::vector<Particle>& NBodySystem::getBodies() { return bodies; }

int    NBodySystem::getCount()   const { return static_cast<int>(bodies.size()); }
double NBodySystem::getG()       const { return G_const; }
double NBodySystem::getEpsilon() const { return softening_eps; }