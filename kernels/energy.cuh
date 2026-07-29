#ifndef ENERGY_CUH
#define ENERGY_CUH

#include <utility>
#include <cuda_runtime.h>
#include "../CudaBuffer.h"

// Firma con punteros crudos
std::pair<double, double> launchComputeEnergyGpu(
    const double* d_x, const double* d_y,
    const double* d_vx, const double* d_vy,
    const double* d_mass,
    int N, double G, double epsilon,
    int method, int block_size = 256);

// Sobrecarga con CudaBuffer
std::pair<double, double> launchComputeEnergyGpu(
    const CudaBuffer<double>& d_x, const CudaBuffer<double>& d_y,
    const CudaBuffer<double>& d_vx, const CudaBuffer<double>& d_vy,
    const CudaBuffer<double>& d_mass,
    int N, double G, double epsilon,
    int method, int block_size = 256);

#endif // ENERGY_CUH