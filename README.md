# Sistemas-distribuidos-y-paralelos-LAB-1

## Requisitos Previos
Si deseas ejecutar el código localmente sin Docker, asegúrate de tener instalado:
* Compilador `g++` con soporte para C++17 y OpenMP (`libomp-dev`).
* Google Test (`libgtest-dev`) para las pruebas unitarias.
* Python 3 con `matplotlib` y `pandas` para la generación de gráficos.
* `ffmpeg` para el renderizado del video de la simulación.

## Ejecución Automatizada con Docker
Para garantizar la reproducibilidad y evitar problemas de dependencias, puedes ejecutar el pipeline completo de pruebas, benchmark y generación de gráficos en un solo comando utilizando Docker con los siguientes comandos:
```bash
docker build -t lab1-nbody .
```
Para hacer el build del contenedor de ubuntu con todo lo necesario para correr el código en la misma carpeta donde se encuentra el proyecto.
Para correr el simulador en linux:
```bash
docker run --rm -v "$(pwd)":/workspace lab1-nbody bash -c "make clean && make && make test && ./nbody_2d && ./nbody_2d -benchmark 1 0 0 0 0 && ./nbody_2d -benchmark 1 0 0 1 10 && ./nbody_2d -benchmark 1 0 0 1 50 && ./nbody_2d -benchmark 1 0 0 1 100 && ./nbody_2d -benchmark 1 0 0 2 10 && ./nbody_2d -benchmark 1 0 0 2 50 && ./nbody_2d -benchmark 1 0 0 2 100 && python3 scripts/plot_energy.py && python3 scripts/plot_scalability.py && python3 scripts/plot_efficiency.py && python3 scripts/plot_chunks.py && python3 scripts/animate_simulation.py"
```
Para correr el simulador en windows:
```bash
docker run --rm -v "%cd%":/workspace lab1-nbody bash -c "make clean && make && make test && ./nbody_2d && ./nbody_2d -benchmark 1 0 0 0 0 && ./nbody_2d -benchmark 1 0 0 1 10 && ./nbody_2d -benchmark 1 0 0 1 50 && ./nbody_2d -benchmark 1 0 0 1 100 && ./nbody_2d -benchmark 1 0 0 2 10 && ./nbody_2d -benchmark 1 0 0 2 50 && ./nbody_2d -benchmark 1 0 0 2 100 && python3 scripts/plot_energy.py && python3 scripts/plot_scalability.py && python3 scripts/plot_efficiency.py && python3 scripts/plot_chunks.py && python3 scripts/animate_simulation.py"
```

## Compilación:

### Compilar
`make`

### Ejecutar benchmarks
`make benchmark`

### Ejecutar an ́alisis
`make analysis`

### Pruebas unitarias e integracion
`make test`

### Limpiar
`make clean`

Instrucciones Detalladas de Ejecución
El simulador cuenta con dos modos principales de ejecución manual si necesitas pasarle parámetros específicos:

### 1. Modo Simulación Estándar (Física y Animación)
Este modo ejecuta la simulación base y genera los archivos necesarios para visualizar el movimiento de las partículas y la conservación de la energía.
```bash
./nbody_2d
```
Este comando ejecuta el simulador y genera los archivos energy_timeseries.dat y trajectories.dat
Para generar los gráficos y el video de las particulas:
```bash
python3 scripts/plot_energy.py
python3 scripts/animate_simulation.py
```
Este comando guardara el video simulation_video.mp4 y el gráfico energy_plot.png.

### 2. Modo Benchmark (Rendimiento y OpenMP)
Este modo realiza pruebas de estrés variando la cantidad de hilos para medir el Speedup, la Eficiencia y evaluar el balanceo de carga (Chunks).
```bash
./nbody_2d -benchmark [task_type] [sync_type] [energy_method] [sched] [chunk]
```
Las variables de este comando y combinaciones son:
- task_type: 0 (Tasks), 1 (Parallel For)
- sync_type: 0 (Atomic), 1 (Critical), 2 (Nowait)
- energy_method: 0 (Reduction), 1 (Atomic)
- sched: 0 (Static), 1 (Dynamic), 2 (Guided)
- chunk: Tamaño del bloque (Ej. 10, 50, 100). Usar 0 para automático.

Una vez ejecutados los benchmarks, para se puede gráficar los resultados con:
```bash
python3 scripts/plot_scalability.py
python3 scripts/plot_efficiency.py
python3 scripts/plot_chunks.py
```

# Sobrecarga y Detalles de Implementación
Utilizada para calcular la desviacion estandar de manera paralela en MetricsCalculator.
Se utiliza first private para poder calcular este valor de manera paralela y ademas asegurar que los valores promedio de los tiempos se entreguen correctamente y que para cada hebra tenga su propia copia local a la hora de ejecutar los calculos
Por defecto se usa la version normal de analyzePerformance, para usarla, cambiar el valor por defecto del metodo runScalabilityTest de mode a 1 en Benchmark.h.
### Ejemplo:
```cpp
simulation_data runScalabilityTest(
        int max_threads, 
        int num_particles, 
        int task_type,      // Nuevo: 0=Task, 1=Parallel For
        int sync_type,      // 0=atomic, 1=critical, 2=nowait
        int energy_method,  // Nuevo: 0=reduce, 1=atomic
        int schedule_type, 
        int chunk_size, 
        double G, 
        double epsilon,
        bool perform_diagnostics = false, //no ejecutara la fucnion de diagnostico con last private.
        int mode = 1 //0= no ejecutara el calculo paralelo de stdev 1= calculo paralelo ejecutado.
    );
