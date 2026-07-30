#ifndef ENERGY_CUH
#define ENERGY_CUH

#include <utility>
#include <cuda_runtime.h>
#include "../CudaBuffer.h"

// Firma con punteros crudos
void launchComputeEnergyGpu(
    const double* d_x, const double* d_y,
    const double* d_vx, const double* d_vy,
    const double* d_mass,
    int N, double G, double epsilon,
    int method, int block_size,
    double* d_u_out, double* d_k_out
);

// Sobrecarga con CudaBuffer
inline void launchComputeEnergyGpu(
    const CudaBuffer<double>& d_x, const CudaBuffer<double>& d_y,
    const CudaBuffer<double>& d_vx, const CudaBuffer<double>& d_vy,
    const CudaBuffer<double>& d_mass,
    int N, double G, double epsilon,
    int method, int block_size,
    CudaBuffer<double>& d_u_out, CudaBuffer<double>& d_k_out);

#endif // ENERGY_CUH