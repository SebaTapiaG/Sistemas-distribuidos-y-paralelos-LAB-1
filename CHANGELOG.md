# Changelog

Todos los cambios notables de este proyecto se documentan aquí.
Formato basado en [Keep a Changelog](https://keepachangelog.com/es/1.0.0/).

---

## [Unreleased]

---

## [2.0.0] - 2026-08-03

### Added
- Kernels CUDA para computeAccelerations (básico y shared memory)
- CudaBuffer: clase RAII para gestión de memoria device
- Layout SoA (Structure of Arrays) en memoria global del device
- Tests de equivalencia CPU vs GPU con tolerancia documentada
- Tres agentes de IA: documentador, revisor de bugs, revisor de MR integrados en CI
- CHANGELOG.md con formato Keep a Changelog
- Protección de rama main (require PR + aprobación)
- Ramas feature/* y fix/* como flujo estándar

### Changed
- Dockerfile actualizado a imagen base nvidia/cuda:12.x-devel-ubuntu22.04
- CI extendido: compilación CUDA + make test en cada MR/PR
- README actualizado con instrucciones GPU, agentes y roles Lab 2

---

## [1.0.0] - 2026-05-08

### Added
- Simulador gravitatorio N-cuerpos 2D con OpenMP (Lab 1)
- Clases: Particle, NBodySystem, NBodySimulator, MetricsCalculator, Benchmark
- Paralelización con cláusulas OpenMP: atomic, critical, reduction, collapse,
  schedule, nowait, barrier, single, task, firstprivate, lastprivate
- Suite de tests con GoogleTest (unitarios e integración)
- Benchmarks: speedup, eficiencia, Amdahl, propagación de errores
- Pipeline CI con GitHub Actions (build + test)
- Dockerfile con Ubuntu + OpenMP + GoogleTest + Python
- Scripts de visualización: energía, escalabilidad, eficiencia, chunks, animación

[Unreleased]: https://github.com/SebaTapiaG/Sistemas-distribuidos-y-paralelos-LAB-1/compare/v2.0.0...HEAD
[2.0.0]: https://github.com/SebaTapiaG/Sistemas-distribuidos-y-paralelos-LAB-1/compare/v1.0.0...v2.0.0
[1.0.0]: https://github.com/SebaTapiaG/Sistemas-distribuidos-y-paralelos-LAB-1/releases/tag/v1.0.0