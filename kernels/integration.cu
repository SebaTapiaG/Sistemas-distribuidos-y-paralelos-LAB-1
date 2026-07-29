#include "integration.cuh"
#include "../CudaBuffer.h"

__global__ void eulerIntegrationKernel(
    double* __restrict__ d_x,
    double* __restrict__ d_y,
    double* __restrict__ d_vx,
    double* __restrict__ d_vy,
    const double* __restrict__ d_ax,
    const double* __restrict__ d_ay,
    int N, double dt)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N) return;

    // 1. Kick: Actualización de velocidades con la aceleración calculada
    double vx = d_vx[i] + d_ax[i] * dt;
    double vy = d_vy[i] + d_ay[i] * dt;

    // 2. Drift: Actualización de posiciones usando la nueva velocidad
    double x = d_x[i] + vx * dt;
    double y = d_y[i] + vy * dt;

    // Escritura en memoria global
    d_vx[i] = vx;
    d_vy[i] = vy;
    d_x[i]  = x;
    d_y[i]  = y;
}

// Lanzador base con punteros crudos
void launchEulerIntegrationGpu(
    double* d_x, double* d_y,
    double* d_vx, double* d_vy,
    const double* d_ax, const double* d_ay,
    int N, double dt, int block_size)
{
    if (N <= 0) return;

    int grid_size = (N + block_size - 1) / block_size;

    eulerIntegrationKernel<<<grid_size, block_size>>>(
        d_x, d_y, d_vx, d_vy, d_ax, d_ay, N, dt
    );

    CUDA_CHECK(cudaGetLastError());
}

// Sobrecarga de alto nivel con CudaBuffer
void launchEulerIntegrationGpu(
    CudaBuffer<double>& d_x, CudaBuffer<double>& d_y,
    CudaBuffer<double>& d_vx, CudaBuffer<double>& d_vy,
    const CudaBuffer<double>& d_ax, const CudaBuffer<double>& d_ay,
    int N, double dt, int block_size)
{
    launchEulerIntegrationGpu(
        d_x.get(), d_y.get(),
        d_vx.get(), d_vy.get(),
        d_ax.get(), d_ay.get(),
        N, dt, block_size
    );
}