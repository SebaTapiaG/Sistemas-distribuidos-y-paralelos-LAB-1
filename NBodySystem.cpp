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
     * se paralelice con OpenMP.
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

// ── Variantes con OpenMP ───────────────────────────────

void NBodySystem::computeAccelerations(int schedule_type, int chunk_size) {
    const int N = static_cast<int>(bodies.size());
    const double eps2 = softening_eps * softening_eps;

    zeroAccelerations();

    // Configurar el schedule dinámicamente según el parámetro
    omp_sched_t kind;
    if (schedule_type == 0) kind = omp_sched_static;
    else if (schedule_type == 1) kind = omp_sched_dynamic;
    else if (schedule_type == 2) kind = omp_sched_guided;
    else kind = omp_sched_auto;

    omp_set_schedule(kind, chunk_size > 0 ? chunk_size : 0);

    // Paralelización del bucle externo 
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
    const int    N    = static_cast<int>(bodies.size());
    const double eps2 = softening_eps * softening_eps;
 
    // Acumuladores externos — evitan condicion de carrera en bodies[i]
    std::vector<double> ax_acc(N, 0.0);
    std::vector<double> ay_acc(N, 0.0);
 
    // collapse(2) fusiona los bucles i y j en un espacio plano de N*N iter.
    // Multiples hilos pueden tener el mismo i con distinto j => atomic obligatorio.
    #pragma omp parallel for collapse(2) default(none) \
            shared(bodies, N, eps2, ax_acc, ay_acc)
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == j) continue;
 
            const double dx    = bodies[j].getX() - bodies[i].getX();
            const double dy    = bodies[j].getY() - bodies[i].getY();
            const double dist2 = dx * dx + dy * dy + eps2;
            const double dist3 = dist2 * std::sqrt(dist2);
            const double fac   = G_const * bodies[j].getMass() / dist3;
 
            #pragma omp atomic
            ax_acc[i] += fac * dx;
            #pragma omp atomic
            ay_acc[i] += fac * dy;
        }
    }
 
    // Escritura final — serial, sin carrera
    for (int i = 0; i < N; ++i)
        bodies[i].setAcceleration(ax_acc[i], ay_acc[i]);
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

//Métodos de Gestión SoA y GPU (Nuevos) ───────────────────────────────
// ── Convierte vector<Particle> (AoS) a Vectores Contiguos SoA (Host) ─────────
void NBodySystem::convertAosToSoa() {
    size_t N = bodies.size();
    h_x.resize(N);
    h_y.resize(N);
    h_vx.resize(N);
    h_vy.resize(N);
    h_mass.resize(N);
    h_ax.resize(N);
    h_ay.resize(N);

    for (size_t i = 0; i < N; ++i) {
        h_x[i]    = bodies[i].getX();
        h_y[i]    = bodies[i].getY();
        h_vx[i]   = bodies[i].getVx();
        h_vy[i]   = bodies[i].getVy();
        h_mass[i] = bodies[i].getMass();
        h_ax[i]   = bodies[i].getAx();
        h_ay[i]   = bodies[i].getAy();
    }
}

// ── Convierte SoA (Host) de vuelta a vector<Particle> (AoS) ──────────────────
void NBodySystem::convertSoaToAos() {
    size_t N = bodies.size();
    for (size_t i = 0; i < N; ++i) {
        bodies[i].setX(h_x[i]);
        bodies[i].setY(h_y[i]);
        bodies[i].setVx(h_vx[i]);
        bodies[i].setVy(h_vy[i]);
        bodies[i].setAx(h_ax[i]);
        bodies[i].setAy(h_ay[i]);
    }
}

// ── Reserva Memoria GPU mediante CudaBuffer ─────────────────────────────────
void NBodySystem::allocateGpuMemory() {
    size_t N = bodies.size();
    if (N == 0) return;

    d_x    = std::make_unique<CudaBuffer<double>>(N);
    d_y    = std::make_unique<CudaBuffer<double>>(N);
    d_vx   = std::make_unique<CudaBuffer<double>>(N);
    d_vy   = std::make_unique<CudaBuffer<double>>(N);
    d_mass = std::make_unique<CudaBuffer<double>>(N);
    d_ax   = std::make_unique<CudaBuffer<double>>(N);
    d_ay   = std::make_unique<CudaBuffer<double>>(N);

    gpu_allocated = true;
}

// ── Transfiere datos Host -> Device ─────────────────────────────────────────
void NBodySystem::copyHostToDevice() {
    if (!gpu_allocated) allocateGpuMemory();

    d_x->ToDevice(h_x.data());
    d_y->ToDevice(h_y.data());
    d_vx->ToDevice(h_vx.data());
    d_vy->ToDevice(h_vy.data());
    d_mass->ToDevice(h_mass.data());
    d_ax->ToDevice(h_ax.data());
    d_ay->ToDevice(h_ay.data());
}

// ── Transfiere datos Device -> Host ─────────────────────────────────────────
void NBodySystem::copyDeviceToHost() {
    if (!gpu_allocated) return;

    d_x->ToHost(h_x.data());
    d_y->ToHost(h_y.data());
    d_vx->ToHost(h_vx.data());
    d_vy->ToHost(h_vy.data());
    d_ax->ToHost(h_ax.data());
    d_ay->ToHost(h_ay.data());
}
// ── Métodos de Cálculo de Aceleraciones en GPU ───────────────────────────────
// Versión 1: Llama a la versión de variant con el valor por defecto 0
void NBodySystem::computeAccelerationsGpu() {
    computeAccelerationsGpu(0, 256);
}

// Versión 2: Llama a la versión completa con block_size por defecto 256
void NBodySystem::computeAccelerationsGpu(int variant) {
    computeAccelerationsGpu(variant, 256);
}

// Versión 3: Firma completa que orquesta la ejecución
void NBodySystem::computeAccelerationsGpu(int variant, int block_size) {
    int N = static_cast<int>(bodies.size());
    if (N == 0) return;

    // Si por alguna razón no se ha asignado memoria antes, la asignamos
    if (!gpu_allocated) {
        convertAosToSoa();
        allocateGpuMemory();
        copyHostToDevice();
    }

    // Invoca el kernel directamente usando los miembros privados G_const y softening_eps
    launchComputeAccelerationsGpu(
        d_x->get(), d_y->get(), d_mass->get(),
        d_ax->get(), d_ay->get(),
        N, G_const, softening_eps, variant, block_size
    );
}
// 1. Constructor de Copia
NBodySystem::NBodySystem(const NBodySystem& other)
    :bodies(other.bodies), 
      G_const(other.G_const), 
      softening_eps(other.softening_eps), 
      h_x(other.h_x), h_y(other.h_y), 
      h_vx(other.h_vx), h_vy(other.h_vy), 
      h_mass(other.h_mass), h_ax(other.h_ax), h_ay(other.h_ay),
      gpu_allocated(false) {

    // Si el objeto origen tenía la GPU inicializada, asignamos y copiamos
    if (other.gpu_allocated) {
        allocateGpuMemory();
        copyHostToDevice();
    }
}

// 2. Operador de Asignación (=)
NBodySystem& NBodySystem::operator=(const NBodySystem& other) {
    if (this != &other) {
        // Copia de miembros CPU
        this->bodies = other.bodies;
        this->G_const = other.G_const;
        this->softening_eps = other.softening_eps;
        this->h_x = other.h_x;
        this->h_y = other.h_y;
        this->h_vx = other.h_vx;
        this->h_vy = other.h_vy;
        this->h_mass = other.h_mass;
        this->h_ax = other.h_ax;
        this->h_ay = other.h_ay;

        // Liberación limpia de los smart pointers anteriores de GPU
        d_x.reset();
        d_y.reset();
        d_vx.reset();
        d_vy.reset();
        d_mass.reset();
        d_ax.reset();
        d_ay.reset();
        gpu_allocated = false;

        // Si el otro objeto tiene la GPU activa, volvemos a reservar y copiar
        if (other.gpu_allocated) {
            allocateGpuMemory();
            copyHostToDevice();
        }
    }
    return *this;
}