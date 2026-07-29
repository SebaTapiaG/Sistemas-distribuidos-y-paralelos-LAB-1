#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <cuda_runtime.h>
#include "../CudaBuffer.h"
#include "../kernels/energy.cuh"

// ── Referencia Serial en CPU ─────────────────────────────────────────────────
std::pair<double, double> computeEnergyCpu(
    const std::vector<double>& x, const std::vector<double>& y,
    const std::vector<double>& vx, const std::vector<double>& vy,
    const std::vector<double>& mass,
    int N, double G, double eps)
{
    double k = 0.0;
    double u = 0.0;
    const double eps2 = eps * eps;

    for (int i = 0; i < N; ++i) {
        // Energía Cinética: 1/2 * m * v^2
        k += 0.5 * mass[i] * (vx[i] * vx[i] + vy[i] * vy[i]);

        // Energía Potencial: Par a par (j < i)
        for (int j = 0; j < i; ++j) {
            const double dx = x[j] - x[i];
            const double dy = y[j] - y[i];
            const double distSq = dx * dx + dy * dy + eps2;
            u += (mass[i] * mass[j]) / std::sqrt(distSq);
        }
    }
    u = -G * u;
    return {u, k};
}

// ── Validación con Tolerancia Flotante ───────────────────────────────────────
bool isClose(double val_gpu, double val_cpu, double rtol = 1e-4, double atol = 1e-8) {
    double diff = std::abs(val_gpu - val_cpu);
    double tolerance = atol + rtol * std::abs(val_cpu);
    return diff <= tolerance;
}

// ── Runner Genérico de Pruebas de Energía ────────────────────────────────────
void runEnergyTest(int N, const std::string& test_name) {
    std::cout << "\n=========================================" << std::endl;
    std::cout << " Ejecutando Test: " << test_name << " (N = " << N << ")" << std::endl;
    std::cout << "=========================================" << std::endl;

    const double G = 1.0;
    const double eps = 0.1;

    // 1. Inicializar datos en Host
    std::vector<double> h_x(N), h_y(N);
    std::vector<double> h_vx(N), h_vy(N);
    std::vector<double> h_mass(N);

    for (int i = 0; i < N; ++i) {
        h_x[i]    = i * 1.2;
        h_y[i]    = i * 0.8;
        h_vx[i]   = 0.5 * (i % 2 == 0 ? 1 : -1);
        h_vy[i]   = 0.3 * (i % 3 == 0 ? 1 : -1);
        h_mass[i] = 1.0 + i * 0.1;
    }

    // 2. Ejecutar cálculo de referencia en CPU
    auto [cpu_U, cpu_K] = computeEnergyCpu(h_x, h_y, h_vx, h_vy, h_mass, N, G, eps);

    // 3. Subir datos a GPU mediante CudaBuffer (RAII)
    CudaBuffer<double> d_x(N), d_y(N);
    CudaBuffer<double> d_vx(N), d_vy(N);
    CudaBuffer<double> d_mass(N);

    d_x.ToDevice(h_x.data());
    d_y.ToDevice(h_y.data());
    d_vx.ToDevice(h_vx.data());
    d_vy.ToDevice(h_vy.data());
    d_mass.ToDevice(h_mass.data());

    // 4. Probar ambos métodos (0: Reducción Compartida, 1: atomicAdd)
    for (int method = 0; method <= 1; ++method) {
        std::string m_name = (method == 0) ? "Reducción Compartida" : "atomicAdd Global";
        std::cout << " -> Probando Método " << m_name << "... ";

        auto [gpu_U, gpu_K] = launchComputeEnergyGpu(
            d_x, d_y, d_vx, d_vy, d_mass,
            N, G, eps, method, 256
        );

        // Validación
        bool ok_u = isClose(gpu_U, cpu_U);
        bool ok_k = isClose(gpu_K, cpu_K);

        if (ok_u && ok_k) {
            std::cout << "EXITOSO" << std::endl;
        } else {
            std::cerr << "\n [FALLO] Discrepancia en " << m_name << ":"
                      << "\n  CPU -> U: " << cpu_U << " | K: " << cpu_K
                      << "\n  GPU -> U: " << gpu_U << " | K: " << gpu_K << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
}

// ── Test Analítico: 1 Partícula (U debe ser 0) ──────────────────────────────
void runAnalyticalSingleParticleTest() {
    std::cout << "\n=========================================" << std::endl;
    std::cout << " Ejecutando Test Analítico: Partícula Aislada (N = 1)" << std::endl;
    std::cout << "=========================================" << std::endl;

    const int N = 1;
    const double G = 1.0, eps = 0.1;

    CudaBuffer<double> d_x(N), d_y(N), d_vx(N), d_vy(N), d_mass(N);

    double h_x = 0.0, h_y = 0.0;
    double h_vx = 3.0, h_vy = 4.0; // v^2 = 25
    double h_mass = 2.0;           // K = 0.5 * 2 * 25 = 25.0

    d_x.ToDevice(&h_x);
    d_y.ToDevice(&h_y);
    d_vx.ToDevice(&h_vx);
    d_vy.ToDevice(&h_vy);
    d_mass.ToDevice(&h_mass);

    for (int method = 0; method <= 1; ++method) {
        auto [gpu_U, gpu_K] = launchComputeEnergyGpu(
            d_x, d_y, d_vx, d_vy, d_mass,
            N, G, eps, method, 256
        );

        if (isClose(gpu_U, 0.0) && isClose(gpu_K, 25.0)) {
            std::cout << " -> Método " << method << ": EXITOSO (U = " << gpu_U << ", K = " << gpu_K << ")" << std::endl;
        } else {
            std::cerr << " [FALLO] Resultado analítico erróneo en método " << method << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
}

int main() {
    // 1. Detección de Hardware
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);

    if (err != cudaSuccess || deviceCount == 0) {
        std::cout << "\n=========================================================" << std::endl;
        std::cout << " [INFO] No se detectó GPU NVIDIA activa." << std::endl;
        std::cout << "        Se omite la ejecución del test de energía." << std::endl;
        std::cout << "=========================================================\n" << std::endl;
        return 0;
    }

    // 2. Ejecutar Casos de Prueba
    runAnalyticalSingleParticleTest();               // Caso Analítico simple
    runEnergyTest(2, "Dos cuerpos (N = 2)");          // Par de partículas
    runEnergyTest(10, "Pequeño sistema (N = 10)");    // Múltiples hilos (1 bloque)
    runEnergyTest(500, "Gran sistema (N = 500)");     // Múltiples bloques (> 256 hilos)

    std::cout << "\n=========================================" << std::endl;
    std::cout << " ¡TODAS LAS PRUEBAS DE ENERGÍA PASARON CON ÉXITO! " << std::endl;
    std::cout << "=========================================\n" << std::endl;
    return 0;
}