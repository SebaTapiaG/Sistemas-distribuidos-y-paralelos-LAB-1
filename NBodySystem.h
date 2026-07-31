#pragma once

#include "Particle.h"
#include <vector>
#include <memory>
#include "CudaBuffer.h"
#include "kernels/accelerations.cuh"

/**
 * NBodySystem
 *
 * Contenedor del sistema gravitatorio N-cuerpos en 2D.
 * Almacena todas las partículas y encapsula los parámetros físicos
 * globales: constante gravitacional G y suavizado de Plummer epsilon.
 *
 * Responsabilidad principal: calcular las aceleraciones de todos los
 * cuerpos aplicando la ley de gravitación newtoniana par a par con
 * suavizado, usando la ecuación:
 *
 *   a_i = G * sum_{j != i} [ mj * (rj - ri) / (|rj - ri|^2 + eps^2)^(3/2) ]
 */
class NBodySystem {
    private:
        std::vector<Particle> bodies;   // Conjunto de partículas del sistema
        double G_const;                 // Constante gravitacional (e.g. G = 1.0)
        double softening_eps;           // Suavizado de Plummer epsilon (evita singularidades)

        // Modificaciones para CUDA: almacenamiento SoA para variables fisicas para su transferencia a GPU
        // Contenedores SoA en Host (CPU)
        std::vector<double> h_x, h_y, h_vx, h_vy, h_mass, h_ax, h_ay;

        // Buffers SoA en Device (GPU) envueltos en CudaBuffer
        std::unique_ptr<CudaBuffer<double>> d_x;
        std::unique_ptr<CudaBuffer<double>> d_y;
        std::unique_ptr<CudaBuffer<double>> d_vx;
        std::unique_ptr<CudaBuffer<double>> d_vy;
        std::unique_ptr<CudaBuffer<double>> d_mass;
        std::unique_ptr<CudaBuffer<double>> d_ax;
        std::unique_ptr<CudaBuffer<double>> d_ay;

        bool gpu_allocated = false;

    public:
        /**
         * Constructor.
         * @param G       Constante gravitacional (usar 1.0 para sistema adimensional)
         * @param epsilon Suavizado de Plummer (recomendado: 0.01 - 0.1 en unidades del sistema)
         */
        NBodySystem(double G, double epsilon);

        // ── Gestión de partículas ────────────────────────────────────────────────

        /** Agrega una partícula al sistema. */
        void addParticle(const Particle& p);
   

        /** Pone ax = ay = 0 en todas las partículas. Llamar antes de computeAccelerations. */
        void zeroAccelerations();

        // ── Cálculo de aceleraciones (versión serial) ─────────────────

        /**
         * Calcula las aceleraciones de todos los cuerpos por interacción par a par.
         * Versión serial de referencia (sin OpenMP).
         * Complejidad: O(N^2) por paso temporal.
         *
         * Convención del enunciado:
         *  - Bucle externo sobre i (cuerpo receptor)
         *  - Bucle interno sobre j != i (cuerpo fuente)
         *  - Solo escribe en a_i, nunca en a_j (evita condiciones de carrera al paralelizar)
         */
        void computeAccelerations();

        /**
         * Variante con tipo de schedule (preparada para OpenMP).
         * @param schedule_type  0 = static, 1 = dynamic, 2 = guided
         */
        void computeAccelerations(int schedule_type);

        /**
         * Variante con schedule y chunk size.
         * @param schedule_type  0 = static, 1 = dynamic, 2 = guided
         * @param chunk_size     Tamaño del chunk de iteraciones por hilo
         */
        void computeAccelerations(int schedule_type, int chunk_size);

        /**
         * Variante con collapse(2) sobre bucles anidados i, j.
         * Requiere demostrar equivalencia con la ecuación de a_i en el reporte.
         */
        void computeAccelerationsCollapse();

        // ── Acceso al estado ─────────────────────────────────────────────────────

        /** Referencia constante al vector de partículas (para métricas, visualización, etc.). */
        const std::vector<Particle>& getBodies() const;

        /** Referencia mutable al vector de partículas (para el integrador). */
        std::vector<Particle>& getBodies();

        /** Constante gravitacional del sistema. */
        double getG() const;

        /** Suavizado de Plummer del sistema. */
        double getEpsilon() const;

        /** Cambia la constante gravitacional del sistema. */
        void setG(double newG);

        /** Cambia el suavizado de Plummer del sistema. */
        void setEpsilon(double newEps);

        // --- CONSTRUCTOR Y OPERADOR DE COPIA (Necesarios para Benchmark) ---
        NBodySystem(const NBodySystem& other);
        NBodySystem& operator=(const NBodySystem& other);


        // ── Métodos de Gestión SoA y GPU (Nuevos) ───────────────
        void convertAosToSoa(); // Llena h_x, h_y, etc., a partir de 'bodies'
        void convertSoaToAos(); // Actualiza 'bodies' a partir de h_x, h_y, h_ax, etc.
        
        void allocateGpuMemory(); // Reserva memoria en GPU vía CudaBuffer
        void copyHostToDevice();  // Sube SoA desde CPU -> GPU
        void copyDeviceToHost();  // Descarga SoA desde GPU -> CPU

        // ── Sobrecargas obligatorias para CUDA ─────────────────────────────────────
        void computeAccelerationsGpu();
        void computeAccelerationsGpu(int variant);
        void computeAccelerationsGpu(int variant, int block_size);

        // Getters para acceder a los punteros raw de la GPU si se requieren fuera
        double* getGpuX() const { return d_x ? d_x->get() : nullptr; }
        double* getGpuY() const { return d_y ? d_y->get() : nullptr; }
        double* getGpuVx() const { return d_vx ? d_vx->get() : nullptr; }
        double* getGpuVy() const { return d_vy ? d_vy->get() : nullptr; }
        double* getGpuMass() const { return d_mass ? d_mass->get() : nullptr; }
        double* getGpuAx() const { return d_ax ? d_ax->get() : nullptr; }
        double* getGpuAy() const { return d_ay ? d_ay->get() : nullptr; }
};