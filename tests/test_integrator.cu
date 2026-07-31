#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cuda_runtime.h>
#include "../CudaBuffer.h"
#include "../kernels/integration.cuh"

// ── Referencia Serial en CPU para Validación ─────────────────────────────────
void computeEulerCpu(
    std::vector<double>& x, std::vector<double>& y,
    std::vector<double>& vx, std::vector<double>& vy,
    const std::vector<double>& ax, const std::vector<double>& ay,
    int N, double dt)
{
    for (int i = 0; i < N; ++i) {
        // Kick
        vx[i] += ax[i] * dt;
        vy[i] += ay[i] * dt;
        // Drift
        x[i] += vx[i] * dt;
        y[i] += vy[i] * dt;
    }
}

// ── Función de Validación de Tolerancia en Coma Flotante ─────────────────────
bool isClose(double val_gpu, double val_cpu, double rtol = 1e-4, double atol = 1e-8) {
    double diff = std::abs(val_gpu - val_cpu);
    double tolerance = atol + rtol * std::abs(val_cpu);
    return diff <= tolerance;
}

// ── Test runner genérico ─────────────────────────────────────────────────────
void runIntegrationTest(int N, double dt, const std::string& test_name) {
    std::cout << "\n=========================================" << std::endl;
    std::cout << " Ejecutando Test: " << test_name << " (N = " << N << ")" << std::endl;
    std::cout << "=========================================" << std::endl;

    // 1. Datos iniciales en Host
    std::vector<double> h_x(N), h_y(N);
    std::vector<double> h_vx(N), h_vy(N);
    std::vector<double> h_ax(N), h_ay(N);

    for (int i = 0; i < N; ++i) {
        h_x[i]  = i * 1.0;
        h_y[i]  = i * -0.5;
        h_vx[i] = 0.1 * (i + 1);
        h_vy[i] = -0.2 * (i + 1);
        h_ax[i] = 9.8 * (i % 2 == 0 ? 1 : -1);
        h_ay[i] = 1.5 * (i % 3 == 0 ? 1 : -1);
    }

    // Copias de referencia para CPU
    std::vector<double> cpu_x  = h_x,  cpu_y  = h_y;
    std::vector<double> cpu_vx = h_vx, cpu_vy = h_vy;

    // 2. Ejecución de Referencia en CPU
    computeEulerCpu(cpu_x, cpu_y, cpu_vx, cpu_vy, h_ax, h_ay, N, dt);

    // 3. Reserva de Memoria y Transferencias con CudaBuffer (RAII)
    CudaBuffer<double> d_x(N), d_y(N);
    CudaBuffer<double> d_vx(N), d_vy(N);
    CudaBuffer<double> d_ax(N), d_ay(N);

    d_x.ToDevice(h_x.data());
    d_y.ToDevice(h_y.data());
    d_vx.ToDevice(h_vx.data());
    d_vy.ToDevice(h_vy.data());
    d_ax.ToDevice(h_ax.data());
    d_ay.ToDevice(h_ay.data());

    // 4. Lanzar Kernel en GPU
    int block_size = 256;
    launchEulerIntegrationGpu(d_x, d_y, d_vx, d_vy, d_ax, d_ay, N, dt, block_size);
    CUDA_CHECK(cudaDeviceSynchronize());

    // 5. Descargar Resultados a Host
    std::vector<double> gpu_x(N), gpu_y(N);
    std::vector<double> gpu_vx(N), gpu_vy(N);

    d_x.ToHost(gpu_x.data());
    d_y.ToHost(gpu_y.data());
    d_vx.ToHost(gpu_vx.data());
    d_vy.ToHost(gpu_vy.data());

    // 6. Validación Elemento a Elemento
    bool passed = true;
    for (int i = 0; i < N; ++i) {
        if (!isClose(gpu_x[i], cpu_x[i]) || !isClose(gpu_y[i], cpu_y[i]) ||
            !isClose(gpu_vx[i], cpu_vx[i]) || !isClose(gpu_vy[i], cpu_vy[i])) 
        {
            passed = false;
            std::cerr << "\n [FALLO] Discrepancia en cuerpo " << i << ":"
                      << "\n  CPU Pos: (" << cpu_x[i] << ", " << cpu_y[i] << ")"
                      << " | GPU Pos: (" << gpu_x[i] << ", " << gpu_y[i] << ")"
                      << "\n  CPU Vel: (" << cpu_vx[i] << ", " << cpu_vy[i] << ")"
                      << " | GPU Vel: (" << gpu_vx[i] << ", " << gpu_vy[i] << ")" 
                      << std::endl;
            break;
        }
    }

    if (passed) {
        std::cout << " -> PASADO CON ÉXITO (rtol=1e-4, atol=1e-8)" << std::endl;
    } else {
        std::exit(EXIT_FAILURE);
    }
}

// ── Test Analítico: Movimiento Rectilíneo Uniforme (Aceleración 0) ───────────
void runAnalyticalTestZeroAcc() {
    std::cout << "\n=========================================" << std::endl;
    std::cout << " Ejecutando Test Analítico: Aceleración Nula" << std::endl;
    std::cout << "=========================================" << std::endl;

    const int N = 1;
    const double dt = 0.5;
    const int block_size = 256;

    CudaBuffer<double> d_x(N), d_y(N), d_vx(N), d_vy(N), d_ax(N), d_ay(N);

    double h_x = 10.0, h_y = 5.0;
    double h_vx = 2.0, h_vy = -4.0;
    double h_ax = 0.0, h_ay = 0.0;

    d_x.ToDevice(&h_x);
    d_y.ToDevice(&h_y);
    d_vx.ToDevice(&h_vx);
    d_vy.ToDevice(&h_vy);
    d_ax.ToDevice(&h_ax);
    d_ay.ToDevice(&h_ay);

    launchEulerIntegrationGpu(d_x, d_y, d_vx, d_vy, d_ax, d_ay, N, dt, block_size);
    CUDA_CHECK(cudaDeviceSynchronize());

    double out_x, out_y, out_vx, out_vy;
    d_x.ToHost(&out_x);
    d_y.ToHost(&out_y);
    d_vx.ToHost(&out_vx);
    d_vy.ToHost(&out_vy);

    // Resultados esperados: vx_new = 2.0, x_new = 10.0 + 2.0 * 0.5 = 11.0
    // vy_new = -4.0, y_new = 5.0 + (-4.0) * 0.5 = 3.0
    if (isClose(out_vx, 2.0) && isClose(out_vy, -4.0) &&
        isClose(out_x, 11.0) && isClose(out_y, 3.0)) 
    {
        std::cout << " -> PASADO CON ÉXITO (x=" << out_x << ", y=" << out_y << ")" << std::endl;
    } else {
        std::cerr << " [FALLO] Resultado analítico incorrecto." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

int main() {
    // 1. Verificar presencia de GPU (CI/CD Fallback)
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);

    if (err != cudaSuccess || deviceCount == 0) {
        std::cout << "\n=========================================================" << std::endl;
        std::cout << " [INFO] No se detectó GPU NVIDIA activa." << std::endl;
        std::cout << "        Se omite la ejecución del test de integración." << std::endl;
        std::cout << "=========================================================\n" << std::endl;
        return 0;
    }

    // 2. Ejecución de casos de prueba
    runAnalyticalTestZeroAcc();                      // Caso Borde Analítico
    runIntegrationTest(1, 0.01, "Caso N = 1");       // Mínimo N
    runIntegrationTest(5, 0.01, "Caso N = 5");       // Pocas partículas
    runIntegrationTest(300, 0.001, "Caso N = 300");  // Múltiples bloques (> 256 hilos)

    std::cout << "\n=========================================" << std::endl;
    std::cout << " ¡TODAS LAS PRUEBAS DE INTEGRACIÓN PASARON! " << std::endl;
    std::cout << "=========================================\n" << std::endl;
    return 0;
}