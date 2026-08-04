# Simulador Gravitatorio N-Cuerpos en 2D (CUDA)

## Introducción
Este proyecto implementa un simulador gravitatorio de N-cuerpos en 2D, cuyo modelo físico calcula la interacción entre partículas mediante la ley de gravitación universal de Newton con suavizado Plummer. Corresponde al Laboratorio 2 de Programación GPGPU, y su objetivo principal es acelerar el coste computacional $\mathcal{O}(N^{2})$ trasladando el núcleo de cálculo a la GPU mediante CUDA C++. 

Además de la paralelización mediante memoria global y compartida (*shared memory*), el proyecto incorpora buenas prácticas de ingeniería de software: integración continua (CI), versionamiento semántico, despliegue en contenedores Docker y la utilización de agentes de Inteligencia Artificial para la validación de código, revisión de Pull Requests y documentación.

> **Nota sobre los resultados:** Los resultados obtenidos (archivos `.dat`, visualizaciones `.png` y análisis de métricas) fueron generados ejecutando el proyecto en el clúster del **DIINF (Departamento de Ingeniería Informática)**, haciendo uso de los nodos de cómputo con GPUs NVIDIA A30.

Para asegurar la reproducibilidad de la física y de las métricas de rendimiento, las pruebas y benchmarks utilizan la **semilla: 42**.

---

## Roles del Equipo

A continuación, se detalla la distribución de roles y responsabilidades principales de los integrantes del equipo:

| Nombre | Rol | Responsabilidades |
| :--- | :--- | :--- |
| **[Vicente Aninat Norambuena]** | **Kernels CUDA** | Desarrollo de `computeAccelerationsKernel` (básico y *shared*), lanzadores host, control de índices y bordes, uso de macros `CUDA_CHECK`. |
| **[Juan Loyola]** | **Host/device y memoria** | Gestión de `CudaBuffer`, diseño de layout SoA en device, operaciones `cudaMalloc`/`cudaMemcpy`/`cudaFree`, minimización de transferencias. |
| **[Rodrigo González García]** | **Integración y validación** | Integración Euler explícita, tests de tolerancia CPU vs GPU, reducción de energía cinética/potencial usando reducción paralela y `atomicAdd`. |
| **[Sebastian Tapia Galeguillos]** | **Git, releases y agentes** | Protección de la rama `main`, gestión del `CHANGELOG.md`, vinculación de issues y configuración de los tres agentes de Inteligencia Artificial (documentador, revisor de bugs, revisor de MR). |
| **[Ignacio Celis Castro]** | **Calidad, CI y visualización**| Extensión del pipeline CI, revisión humana de PRs, imagen Docker, ejecución en clúster y generación de scripts Python para gráficos de rendimiento. |

---
### Aclaraciones:
* El tiempo es medido en segundos y transformado a milisegundos
* Las tolerancias usadas son las recomendadas como punto inicial en el enunciado y no se vio necesaria su modificación, ya que no se apreciaba una perturbación en la física del sistema

### Archivos de Salida
---
| Archivo                       | Contenido                                                                   |
| ----------------------------- | --------------------------------------------------------------------------- |
| `benchmark_results.dat`       | Matriz maestra: N, variant, blockSize, tiempos CPU/GPU, speedups, accuracy. |
| `scaling_analysis.dat`        | Subconjunto para análisis de escalabilidad (filtrado por block size fijo).  |
| `blockdim_study.dat`          | Subconjunto para estudio de blockDim.x (filtrado por N fijo).               |
| `cluster_run.log` / `cluster_sim_run.log` | Logs de Slurm: nodo, GPU, driver, `nvcc --version`, comandos.               |
| `energy_timeseries.dat`       | Historia de K, U, E_total por paso (modo `-sim`).                           |
| `trajectories.dat`            | Trayectorias: paso, ID, masa, x, y, vx, vy (modo `-sim`).                   |


## Requisitos del Sistema

### Hardware
* Procesador multi-núcleo (CPU).
* Tarjeta Gráfica (GPU) NVIDIA A30 (Arquitectura sm80)

### Software y Librerías
* **Sistema Operativo:** Linux (Ubuntu 22.04 recomendado).
* **Contenedores:** Imagen base `nvidia/cuda:12.2.2-devel-ubuntu22.04` (usada en entorno SLURM).
* **Compiladores:** `g++` con soporte para C++17 y `nvcc` (CUDA Toolkit 12.2.2 o superior).
* **Dependencias C++:** `make`, `libomp-dev`.
* **Entorno Python:** Python 3.x para la generación de gráficos.
* **Librerías Python:** `matplotlib`, `numpy`, `pandas`.

---

## Instalación de los requisitos

1. **Clonar el repositorio:**
   ```bash
   git https://github.com/SebaTapiaG/Sistemas-distribuidos-y-paralelos-LAB-1.git
   ```

2. **Instalar dependencias del sistema y Python (basado en Ubuntu/Debian):**
   ```bash
   sudo apt-get update
   sudo apt-get install -y --no-install-recommends g++ make libomp-dev python3 python3-pip ffmpeg
   pip install matplotlib numpy pandas
   ```

---

## Compilación y Ejecución

### Compilación
Para compilar el proyecto enfocado a la GPU del clúster (arquitectura `sm_80` para NVIDIA A30), ejecuta los siguientes comandos extraídos de la configuración SLURM:

```bash
make clean
make NVCCFLAGS='-O3 -std=c++17 -Xcompiler -Wall,-Wextra -arch=sm_80'
```

### Ejecución (Ejemplo de Uso)
* Una vez compilado, para hacer una ejecución de la matriz de pruebas:

```bash
./nbody_2d_cuda -suite
```
#### Parámetros de -suite
---
| Posición (argv) | Parámetro          | Tipo  | Valor por defecto | Descripción                                                |
| --------------- | ------------------ | ----- | ----------------- | ---------------------------------------------------------- |
| `argv[1]`       | `-suite`           | flag  | —                 | Activa el modo suite completa                              |
| `argv[2]`       | `runs`             | `int` | `10`              | Repeticiones por prueba                                    |
| `argv[3]`       | `fixed_block_size` | `int` | `0`               | Filtro blockDim para `scaling_analysis` (`0` = sin filtro) |
| `argv[4]`       | `fixed_n`          | `int` | `0`               | Filtro N para `blockdim_study` (`0` = sin filtro)          |

* Para correr una simulación, exportando energía y posiciones:
```bash
./nbody_2d_cuda -sim
```
#### Parámetros de -sim
---
| Posición (argv) | Parámetro       | Tipo  | Valor por defecto | Descripción                       |
| --------------- | --------------- | ----- | ----------------- | --------------------------------- |
| `argv[1]`       | `-sim`          | flag  | —                 | Activa el modo simulación         |
| `argv[2]`       | `N`             | `int` | `500`             | Número de partículas              |
| `argv[3]`       | `steps`         | `int` | `5000`            | Pasos temporales de la simulación |
| `argv[4]`       | `variant`       | `int` | `0`               | Variante del kernel GPU           |
| `argv[5]`       | `block_size`    | `int` | `256`             | Tamaño de bloque CUDA             |
| `argv[6]`       | `energy_method` | `int` | `0`               | Método de cálculo de energía      |
* Para ejecutar un test individual:
```bash
./nbody_2d_cuda -test
```
#### Parámetros de -test
---

| Posición (argv) | Parámetro    | Tipo  | Valor por defecto | Descripción                          |
| --------------- | ------------ | ----- | ----------------- | ------------------------------------ |
| `argv[1]`       | `-test`      | flag  | —                 | Activa el modo test individual       |
| `argv[2]`       | `N`          | `int` | `1024`            | Número de partículas                 |
| `argv[3]`       | `variant`    | `int` | `0`               | Variante del kernel GPU (0 para global, 1 para shared)             |
| `argv[4]`       | `block_size` | `int` | `256`             | Tamaño de bloque CUDA (`blockDim.x`) |
| `argv[5]`       | `runs`       | `int` | `10`              | Corridas para promediar tiempos      |


*Para correr el suite de pruebas en el Cluster se utiliza el Slurm " run_docker.slurm", ejecutando el siguiente comando:*
```bash
sbatch run_docker.slurm
```
*Su equivalente para simular y almacenar las energias y posiciones es:*
```bash
sbatch run_sim.slurm
```

### Generación de Gráficos
Para generar las visualizaciones (archivos `.png`) a partir de los `.dat` resultantes, utiliza los scripts de Python dispuestos en el directorio `scripts/`. **Debes ejecutarlos desde la carpeta raíz del proyecto** de la siguiente forma:

**Generar todas las visualizaciones y datos de una vez:**
```bash
python scripts/generate_all_plots.py
```

**O ejecutar scripts individuales según el análisis deseado:**
```bash
python scripts/animate_simulation.py
python scripts/plot_amdahl.py
python scripts/plot_basic_vs_shared.py
python scripts/plot_blockdim.py
python scripts/plot_energy.py
python scripts/plot_kernel_vs_endtoend.py
python scripts/plot_scalability.py
python scripts/plot_speedup_gpu.py
```