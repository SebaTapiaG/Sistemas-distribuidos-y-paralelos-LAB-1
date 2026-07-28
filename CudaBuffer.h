#ifndef CUDA_BUFFER_H
#define CUDA_BUFFER_H

#include <cuda_runtime.h>
#include <iostream>
#include <stdexcept>

// Macro obligatoria solicitada en el laboratorio para envolver las APIs de CUDA
#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA error en " << __FILE__ << ":" << __LINE__ \
                      << " código=" << err << " \"" << cudaGetErrorString(err) << "\"" << std::endl; \
            throw std::runtime_error("Error de CUDA"); \
        } \
    } while (0)

template <typename T>
class CudaBuffer {
private:
    T* d_ptr;
    size_t num_elements;

public:
    // Constructor
    explicit CudaBuffer(size_t size) : num_elements(size), d_ptr(nullptr) {
        if (size > 0) {
            CUDA_CHECK(cudaMalloc(&d_ptr, size * sizeof(T)));
        }
    }

    // Destructor
    ~CudaBuffer() {
        if (d_ptr != nullptr) {
            cudaFree(d_ptr); // No se usa CUDA_CHECK aquí para evitar lanzar excepciones en destructores
            d_ptr = nullptr;
        }
    }

    // ========================================================================
    // Evitar copias accidentales que generarían un "double free" en la GPU
    // ========================================================================
    CudaBuffer(const CudaBuffer&) = delete;
    CudaBuffer& operator=(const CudaBuffer&) = delete;

    // ========================================================================
    // Permitir semántica de movimiento (Move semantics)
    // ========================================================================
    CudaBuffer(CudaBuffer&& other) noexcept : d_ptr(other.d_ptr), num_elements(other.num_elements) {
        other.d_ptr = nullptr;
        other.num_elements = 0;
    }

    CudaBuffer& operator=(CudaBuffer&& other) noexcept {
        if (this != &other) {
            if (d_ptr) cudaFree(d_ptr);
            d_ptr = other.d_ptr;
            num_elements = other.num_elements;
            other.d_ptr = nullptr;
            other.num_elements = 0;
        }
        return *this;
    }

    // ========================================================================
    // Transferencias Host/Device
    // ========================================================================
    
    // H2D: Copiar desde el Host (CPU) hacia el Device (GPU)
    void ToDevice(const T* h_ptr) {
        if (num_elements > 0) {
            CUDA_CHECK(cudaMemcpy(d_ptr, h_ptr, num_elements * sizeof(T), cudaMemcpyHostToDevice));
        }
    }

    // D2H: Copiar desde el Device (GPU) hacia el Host (CPU)
    void ToHost(T* h_ptr) const {
        if (num_elements > 0) {
            CUDA_CHECK(cudaMemcpy(h_ptr, d_ptr, num_elements * sizeof(T), cudaMemcpyDeviceToHost));
        }
    }

    // ========================================================================
    // Accesos
    // ========================================================================
    
    // Obtener el puntero crudo del device para pasarlo a los kernels CUDA <<<grid, block>>>
    T* get() const {
        return d_ptr;
    }
    
    // Obtener la cantidad de elementos en el buffer
    size_t size() const {
        return num_elements;
    }
};

#endif // CUDA_BUFFER_H