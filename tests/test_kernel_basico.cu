#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cuda_runtime.h>
#include "../kernels/accelerations.cuh"

// Cálculo de referencia en CPU
void computeAccelerationsCpu(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::vector<double>& mass,
    std::vector<double>& ax,
    std::vector<double>& ay,
    int N, double G, double eps)
{
    const double eps2 = eps * eps;
    for (int i = 0; i < N; ++i) {
        ax[i] = 0.0;
        ay[i] = 0.0;
        for (int j = 0; j < N; ++j) {
            if (i == j) continue;
            const double dx = x[j] - x[i];
            const double dy = y[j] - y[i];
            const double dist2 = dx * dx + dy * dy + eps2;
            const double dist3 = dist2 * std::sqrt(dist2);
            const double factor = G * mass[j] / dist3;
            ax[i] += factor * dx;
            ay[i] += factor * dy;
        }
    }
}

// Función de validación de tolerancias rtol y atol
bool isClose(double val_gpu, double val_cpu, double rtol = 1e-4, double atol = 1e-8) {
    double diff = std::abs(val_gpu - val_cpu);
    double tolerance = atol + rtol * std::abs(val_cpu);
    return diff <= tolerance;
}

void runTestCase(int N, const std::string& label) {
    std::cout << "\n=========================================" << std::endl;
    std::cout << " Ejecutando Test: " << label << " (N = " << N << ")" << std::endl;
    std::cout << "=========================================" << std::endl;

    const double G = 1.0;
    const double eps = 0.1;

    // Inicialización de datos
    std::vector<double> h_x(N), h_y(N), h_mass(N);
    for (int i = 0; i < N; ++i) {
        h_x[i] = i * 1.5;
        h_y[i] = i * 0.5;
        h_mass[i] = 1.0 + i * 0.2;
    }

    std::vector<double> cpu_ax(N, 0.0), cpu_ay(N, 0.0);
    std::vector<double> gpu_ax(N, 0.0), gpu_ay(N, 0.0);

    // 1. Ejecución en CPU
    computeAccelerationsCpu(h_x, h_y, h_mass, cpu_ax, cpu_ay, N, G, eps);

    // 2. Ejecución en GPU (SoA)
    double *d_x, *d_y, *d_mass, *d_ax, *d_ay;
    CUDA_CHECK(cudaMalloc(&d_x, N * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_y, N * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_mass, N * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_ax, N * sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_ay, N * sizeof(double)));

    CUDA_CHECK(cudaMemcpy(d_x, h_x.data(), N * sizeof(double), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_y, h_y.data(), N * sizeof(double), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_mass, h_mass.data(), N * sizeof(double), cudaMemcpyHostToDevice));

    // Probar ambas variantes de kernel (0: Básico, 1: Shared Memory)
    for (int variant = 0; variant <= 1; ++variant) {
        std::string v_name = (variant == 0) ? "Básico Global" : "Memoria Compartida";
        std::cout << " -> Probando Kernel " << v_name << "... ";

        launchComputeAccelerationsGpu(d_x, d_y, d_mass, d_ax, d_ay, N, G, eps, variant, 256);
        CUDA_CHECK(cudaDeviceSynchronize());

        CUDA_CHECK(cudaMemcpy(gpu_ax.data(), d_ax, N * sizeof(double), cudaMemcpyDeviceToHost));
        CUDA_CHECK(cudaMemcpy(gpu_ay.data(), d_ay, N * sizeof(double), cudaMemcpyDeviceToHost));

        // Validación elemento a elemento
        bool passed = true;
        for (int i = 0; i < N; ++i) {
            if (!isClose(gpu_ax[i], cpu_ax[i]) || !isClose(gpu_ay[i], cpu_ay[i])) {
                passed = false;
                std::cerr << "\n [FALLO] Discrepancia en particula " << i 
                          << "\n  CPU ax: " << cpu_ax[i] << " | GPU ax: " << gpu_ax[i]
                          << "\n  CPU ay: " << cpu_ay[i] << " | GPU ay: " << gpu_ay[i] << std::endl;
                break;
            }
        }

        if (passed) {
            std::cout << "EXITOSO (rtol=1e-4, atol=1e-8)" << std::endl;
        } else {
            std::exit(EXIT_FAILURE);
        }
    }

    CUDA_CHECK(cudaFree(d_x));
    CUDA_CHECK(cudaFree(d_y));
    CUDA_CHECK(cudaFree(d_mass));
    CUDA_CHECK(cudaFree(d_ax));
    CUDA_CHECK(cudaFree(d_ay));
}

int main() {
    runTestCase(2, "Test N = 2");
    runTestCase(3, "Test N = 3");

    std::cout << "\n=========================================" << std::endl;
    std::cout << " TESTS COMPLETADOS EXITOSAMENTE " << std::endl;
    std::cout << "=========================================\n" << std::endl;
    return 0;
}