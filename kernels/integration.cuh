#ifndef INTEGRATION_CUH
#define INTEGRATION_CUH

#include <cuda_runtime.h>
#include "../CudaBuffer.h"

void launchEulerIntegrationGpu(
    double* d_x, double* d_y,
    double* d_vx, double* d_vy,
    const double* d_ax, const double* d_ay,
    int N, double dt, int block_size = 256);

void launchEulerIntegrationGpu(
    CudaBuffer<double>& d_x, CudaBuffer<double>& d_y,
    CudaBuffer<double>& d_vx, CudaBuffer<double>& d_vy,
    const CudaBuffer<double>& d_ax, const CudaBuffer<double>& d_ay,
    int N, double dt, int block_size = 256);

#endif // INTEGRATION_CUH