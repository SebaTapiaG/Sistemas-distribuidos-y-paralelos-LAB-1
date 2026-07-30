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

// ── Kernel Auxiliar: Consolidación final de bloques en GPU (para Método 0) ──
__global__ void aggregateEnergyBlocksKernel(
    const double* __restrict__ d_block_U,
    const double* __restrict__ d_block_K,
    int num_blocks,
    double* __restrict__ d_u_out,
    double* __restrict__ d_k_out)
{
    extern __shared__ double sdata[];
    double* s_u = sdata;
    double* s_k = sdata + blockDim.x;

    int tid = threadIdx.x;
    double sum_u = 0.0;
    double sum_k = 0.0;

    // Acumular múltiples bloques si num_blocks > blockDim.x
    for (int i = tid; i < num_blocks; i += blockDim.x) {
        sum_u += d_block_U[i];
        sum_k += d_block_K[i];
    }

    s_u[tid] = sum_u;
    s_k[tid] = sum_k;
    __syncthreads();

    // Reducción dentro del bloque
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            s_u[tid] += s_u[tid + s];
            s_k[tid] += s_k[tid + s];
        }
        __syncthreads();
    }

    // El hilo 0 escribe el resultado final directamente en la memoria de la GPU
    if (tid == 0) {
        *d_u_out = s_u[0];
        *d_k_out = s_k[0];
    }
}

// ── Lanzador Host Unificado (100% GPU / Sin ToHost) ─────────────────────────
void launchComputeEnergyGpu(
    const double* d_x, const double* d_y,
    const double* d_vx, const double* d_vy,
    const double* d_mass,
    int N, double G, double epsilon,
    int method, int block_size,
    double* d_u_out, double* d_k_out)
{
    if (N <= 0) return;

    const double eps2 = epsilon * epsilon;
    const int grid_size = (N + block_size - 1) / block_size;

    if (method == 0) {
        // Reducción paralela
        CudaBuffer<double> d_block_U(grid_size);
        CudaBuffer<double> d_block_K(grid_size);

        size_t shared_bytes = 2 * block_size * sizeof(double);

        computeEnergyKernelReduction<<<grid_size, block_size, shared_bytes>>>(
            d_x, d_y, d_vx, d_vy, d_mass,
            d_block_U.get(), d_block_K.get(),
            N, G, eps2
        );

        int reduce_threads = 256;
        size_t reduce_shared_bytes = 2 * reduce_threads * sizeof(double);
        
        aggregateEnergyBlocksKernel<<<1, reduce_threads, reduce_shared_bytes>>>(
            d_block_U.get(), d_block_K.get(),
            grid_size, d_u_out, d_k_out
        );
    } else {
        // atomicAdd
        cudaMemset(d_u_out, 0, sizeof(double));
        cudaMemset(d_k_out, 0, sizeof(double));

        computeEnergyKernelAtomic<<<grid_size, block_size>>>(
            d_x, d_y, d_vx, d_vy, d_mass,
            d_u_out, d_k_out,
            N, G, eps2
        );
    }
}

inline void launchComputeEnergyGpu(
    const CudaBuffer<double>& d_x, const CudaBuffer<double>& d_y,
    const CudaBuffer<double>& d_vx, const CudaBuffer<double>& d_vy,
    const CudaBuffer<double>& d_mass,
    int N, double G, double epsilon,
    int method, int block_size,
    CudaBuffer<double>& d_u_out, CudaBuffer<double>& d_k_out)
{
    launchComputeEnergyGpu(
        d_x.get(), d_y.get(),
        d_vx.get(), d_vy.get(),
        d_mass.get(),
        N, G, epsilon,
        method, block_size,
        d_u_out.get(), d_k_out.get()
    );
}