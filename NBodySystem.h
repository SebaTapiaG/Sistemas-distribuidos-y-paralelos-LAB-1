#pragma once

#include "Particle.h"
#include <vector>

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
 *
 * En semanas posteriores se añadirán variantes paralelas con OpenMP
 * (schedules, collapse, etc.) mediante sobrecarga de computeAccelerations.
 */
class NBodySystem {
    private:
        std::vector<Particle> bodies;   // Conjunto de partículas del sistema
        double G_const;                 // Constante gravitacional (e.g. G = 1.0)
        double softening_eps;           // Suavizado de Plummer epsilon (evita singularidades)

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

        // ── Cálculo de aceleraciones (versión serial — semana 1) ─────────────────

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
         * Variante con tipo de schedule (preparada para OpenMP — semana 2).
         * @param schedule_type  0 = static, 1 = dynamic, 2 = guided
         */
        void computeAccelerations(int schedule_type);

        /**
         * Variante con schedule y chunk size (semana 2).
         * @param schedule_type  0 = static, 1 = dynamic, 2 = guided
         * @param chunk_size     Tamaño del chunk de iteraciones por hilo
         */
        void computeAccelerations(int schedule_type, int chunk_size);

        /**
         * Variante con collapse(2) sobre bucles anidados i, j (semana 2).
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
};