#include "energy.cuh"
#include "../CudaBuffer.h"
#include <cmath>
#include <numeric>
#include <vector>
#include <cuda_runtime.h>

// ── Kernel Método 0: Reducción Paralela (Árbol en __shared__, sin atomicAdd) ─
__global__ void computeEnergyKernelReduction(
    const double* __restrict__ d_x,
    const double* __restrict__ d_y,
    const double* __restrict__ d_vx,
    const double* __restrict__ d_vy,
    const double* __restrict__ d_mass,
    double* __restrict__ d_block_U,
    double* __restrict__ d_block_K,
    int N, double G, double eps2)
{
    extern __shared__ double s_mem[];
    double* s_u = s_mem;
    double* s_k = &s_mem[blockDim.x];

    const int tid = threadIdx.x;
    const int i   = blockIdx.x * blockDim.x + tid;

    double local_u = 0.0;
    double local_k = 0.0;

    if (i < N) {
        const double xi  = d_x[i];
        const double yi  = d_y[i];
        const double vxi = d_vx[i];
        const double vyi = d_vy[i];
        const double mi  = d_mass[i];

        // 1. Energía Cinética local
        local_k = 0.5 * mi * (vxi * vxi + vyi * vyi);

        // 2. Energía Potencial local (interacción par a par)
        double sum_pot = 0.0;
        for (int j = 0; j < N; ++j) {
            if (j == i) continue;

            const double dx = d_x[j] - xi;
            const double dy = d_y[j] - yi;
            const double dist2 = dx * dx + dy * dy + eps2;

            sum_pot += d_mass[j] / sqrt(dist2);
        }
        local_u = -0.5 * G * mi * sum_pot;
    }

    s_u[tid] = local_u;
    s_k[tid] = local_k;
    __syncthreads();

    // 3. Reducción en árbol binario dentro de la memoria compartida
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            s_u[tid] += s_u[tid + s];
            s_k[tid] += s_k[tid + s];
        }
        __syncthreads();
    }

    // El primer hilo del bloque escribe el total parcial del bloque
    if (tid == 0) {
        d_block_U[blockIdx.x] = s_u[0];
        d_block_K[blockIdx.x] = s_k[0];
    }
}

// ── Kernel Método 1: Acumulación Atómica Directa (atomicAdd) ────────────────
__global__ void computeEnergyKernelAtomic(
    const double* __restrict__ d_x,
    const double* __restrict__ d_y,
    const double* __restrict__ d_vx,
    const double* __restrict__ d_vy,
    const double* __restrict__ d_mass,
    double* __restrict__ d_U_total,
    double* __restrict__ d_K_total,
    int N, double G, double eps2)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N) return;

    const double xi  = d_x[i];
    const double yi  = d_y[i];
    const double vxi = d_vx[i];
    const double vyi = d_vy[i];
    const double mi  = d_mass[i];

    // Cómputo local
    double local_k = 0.5 * mi * (vxi * vxi + vyi * vyi);

    double sum_pot = 0.0;
    for (int j = 0; j < N; ++j) {
        if (j == i) continue;

        const double dx = d_x[j] - xi;
        const double dy = d_y[j] - yi;
        const double dist2 = dx * dx + dy * dy + eps2;

        sum_pot += d_mass[j] / sqrt(dist2);
    }
    double local_u = -0.5 * G * mi * sum_pot;

    // Suma atómica directa a la memoria global
    atomicAdd(d_U_total, local_u);
    atomicAdd(d_K_total, local_k);
}

// ── Lanzador Host Unificado ──────────────────────────────────────────────────
std::pair<double, double> launchComputeEnergyGpu(
    const double* d_x, const double* d_y,
    const double* d_vx, const double* d_vy,
    const double* d_mass,
    int N, double G, double epsilon,
    int method, int block_size)
{
    if (N <= 0) return {0.0, 0.0};

    const double eps2 = epsilon * epsilon;
    const int grid_size = (N + block_size - 1) / block_size;

    double h_U = 0.0, h_K = 0.0;

    if (method == 0) {
        // ── MÉTODO 0: Reducción Paralela
        CudaBuffer<double> d_block_U(grid_size);
        CudaBuffer<double> d_block_K(grid_size);

        size_t shared_bytes = 2 * block_size * sizeof(double);

        computeEnergyKernelReduction<<<grid_size, block_size, shared_bytes>>>(
            d_x, d_y, d_vx, d_vy, d_mass,
            d_block_U.get(), d_block_K.get(),
            N, G, eps2
        );

        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());

        // Descargar sumas parciales por bloque y consolidar en CPU
        std::vector<double> h_block_U(grid_size), h_block_K(grid_size);
        d_block_U.ToHost(h_block_U.data());
        d_block_K.ToHost(h_block_K.data());

        h_U = std::accumulate(h_block_U.begin(), h_block_U.end(), 0.0);
        h_K = std::accumulate(h_block_K.begin(), h_block_K.end(), 0.0);

    } else {
        // ── MÉTODO 1: atomicAdd Directo
        CudaBuffer<double> d_U(1);
        CudaBuffer<double> d_K(1);

        double zero = 0.0;
        d_U.ToDevice(&zero);
        d_K.ToDevice(&zero);

        computeEnergyKernelAtomic<<<grid_size, block_size>>>(
            d_x, d_y, d_vx, d_vy, d_mass,
            d_U.get(), d_K.get(),
            N, G, eps2
        );

        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());

        d_U.ToHost(&h_U);
        d_K.ToHost(&h_K);
    }

    return {h_U, h_K};
}

// Sobrecarga de alto nivel con CudaBuffer
std::pair<double, double> launchComputeEnergyGpu(
    const CudaBuffer<double>& d_x, const CudaBuffer<double>& d_y,
    const CudaBuffer<double>& d_vx, const CudaBuffer<double>& d_vy,
    const CudaBuffer<double>& d_mass,
    int N, double G, double epsilon,
    int method, int block_size)
{
    return launchComputeEnergyGpu(
        d_x.get(), d_y.get(),
        d_vx.get(), d_vy.get(),
        d_mass.get(),
        N, G, epsilon, method, block_size
    );
}