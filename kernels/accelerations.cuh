#ifndef ACCELERATIONS_CUH
#define ACCELERATIONS_CUH

#include "../CudaBuffer.h"
#include <cstdio>
#include <cstdlib>

// ── Kernels CUDA ─────────────────────────────────────────────────────────────

/**
 * Kernel Básico: Un hilo por partícula i.
 * Recorre de forma serial todos los cuerpos j != i en memoria global.
 */
__global__ void computeAccelerationsKernelBasic(
    const double* __restrict__ d_x,
    const double* __restrict__ d_y,
    const double* __restrict__ d_mass,
    double* __restrict__ d_ax,
    double* __restrict__ d_ay,
    int N, double G, double eps2);

/**
 * Kernel con Memoria Compartida (__shared__):
 * Carga bloques (tiles) de partículas en __shared__ memory para reducir
 * los accesos a memoria global. Usa __syncthreads() para coordinación.
 */
__global__ void computeAccelerationsKernelShared(
    const double* __restrict__ d_x,
    const double* __restrict__ d_y,
    const double* __restrict__ d_mass,
    double* __restrict__ d_ax,
    double* __restrict__ d_ay,
    int N, double G, double eps2);

// ── Lanzadores Host (Host Launchers) ─────────────────────────────────────────

/**
 * Lanza el kernel de aceleraciones según la variante seleccionada:
 * @param d_x, d_y, d_mass Punteros a arreglos de entrada en memoria del device (SoA).
 * @param d_ax, d_ay      Punteros a arreglos de salida en memoria del device (SoA).
 * @param N               Número total de cuerpos.
 * @param G               Constante gravitacional.
 * @param epsilon         Suavizado de Plummer.
 * @param variant         0 = Básico, 1 = Shared Memory.
 * @param block_size      Tamaño del bloque CUDA (ej: 64, 128, 256, 512, 1024).
 */
void launchComputeAccelerationsGpu(
    const double* d_x, const double* d_y, const double* d_mass,
    double* d_ax, double* d_ay,
    int N, double G, double epsilon,
    int variant = 0, int block_size = 256);

#endif // ACCELERATIONS_CUH