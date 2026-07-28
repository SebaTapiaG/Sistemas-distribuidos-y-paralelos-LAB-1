#include "accelerations.cuh"
#include <cmath>
#include <iostream>

// ── Kernel 1: Variante Básica (Memoria Global) ───────────────────────────────

__global__ void computeAccelerationsKernelBasic(
    const double* __restrict__ d_x,
    const double* __restrict__ d_y,
    const double* __restrict__ d_mass,
    double* __restrict__ d_ax,
    double* __restrict__ d_ay,
    int N, double G, double eps2)
{
    // Mapeo ID de hilo a cuerpo i
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    
    // Guardia de límites
    if (i >= N) return;

    const double xi = d_x[i];
    const double yi = d_y[i];
    double acc_x = 0.0;
    double acc_y = 0.0;

    // Bucle interno serial sobre j != i
    for (int j = 0; j < N; ++j) {
        if (j == i) continue;

        const double dx = d_x[j] - xi;
        const double dy = d_y[j] - yi;
        
        // Distancia suavizada al cuadrado: |rj - ri|^2 + eps^2
        const double dist2 = dx * dx + dy * dy + eps2;
        const double dist3 = dist2 * sqrt(dist2);
        
        const double factor = G * d_mass[j] / dist3;

        acc_x += factor * dx;
        acc_y += factor * dy;
    }

    d_ax[i] = acc_x;
    d_ay[i] = acc_y;
}

// ── Kernel 2: Variante con Memoria Compartida (__shared__) ───────────────────

__global__ void computeAccelerationsKernelShared(
    const double* __restrict__ d_x,
    const double* __restrict__ d_y,
    const double* __restrict__ d_mass,
    double* __restrict__ d_ax,
    double* __restrict__ d_ay,
    int N, double G, double eps2)
{
    // Reserva de memoria compartida dinámica para (x, y, masa) del tile
    extern __shared__ double s_mem[];
    double* s_x = s_mem;
    double* s_y = &s_mem[blockDim.x];
    double* s_m = &s_mem[2 * blockDim.x];

    const int tid = threadIdx.x;
    const int i   = blockIdx.x * blockDim.x + tid;

    double xi = 0.0, yi = 0.0;
    if (i < N) {
        xi = d_x[i];
        yi = d_y[i];
    }

    double acc_x = 0.0;
    double acc_y = 0.0;

    const int numTiles = (N + blockDim.x - 1) / blockDim.x;

    for (int tile = 0; tile < numTiles; ++tile) {
        // Carga cooperativa del tile a memoria compartida
        const int j_global = tile * blockDim.x + tid;
        if (j_global < N) {
            s_x[tid] = d_x[j_global];
            s_y[tid] = d_y[j_global];
            s_m[tid] = d_mass[j_global];
        } else {
            s_x[tid] = 0.0;
            s_y[tid] = 0.0;
            s_m[tid] = 0.0;
        }

        // Sincronización obligatoria tras la carga del tile
        __syncthreads();

        // Cómputo de fuerzas para los hilos válidos
        if (i < N) {
            #pragma unroll 8
            for (int k = 0; k < blockDim.x; ++k) {
                const int j = tile * blockDim.x + k;
                if (j >= N) break;      // Fuera del rango total de partículas
                if (j == i) continue;  // Omitir auto-interacción

                const double dx = s_x[k] - xi;
                const double dy = s_y[k] - yi;
                const double dist2 = dx * dx + dy * dy + eps2;
                const double dist3 = dist2 * sqrt(dist2);
                const double factor = G * s_m[k] / dist3;

                acc_x += factor * dx;
                acc_y += factor * dy;
            }
        }

        // Sincronización antes de cargar el siguiente tile
        __syncthreads();
    }

    // Escritura en memoria global
    if (i < N) {
        d_ax[i] = acc_x;
        d_ay[i] = acc_y;
    }
}

// ── Lanzador Host ────────────────────────────────────────────────────────────

void launchComputeAccelerationsGpu(
    const double* d_x, const double* d_y, const double* d_mass,
    double* d_ax, double* d_ay,
    int N, double G, double epsilon,
    int variant, int block_size)
{
    if (N <= 0) return;

    const double eps2 = epsilon * epsilon;
    const int grid_size = (N + block_size - 1) / block_size;

    if (variant == 0) {
        // Variante 0: Kernel Básico
        computeAccelerationsKernelBasic<<<grid_size, block_size>>>(
            d_x, d_y, d_mass, d_ax, d_ay, N, G, eps2);
    } else {
        // Variante 1: Kernel con Memoria Compartida
        size_t shared_bytes = 3 * block_size * sizeof(double);
        computeAccelerationsKernelShared<<<grid_size, block_size, shared_bytes>>>(
            d_x, d_y, d_mass, d_ax, d_ay, N, G, eps2);
    }

    // Verificación de errores de lanzamiento y sincronización
    CUDA_CHECK(cudaGetLastError());
}