# Sistemas-distribuidos-y-paralelos-LAB-1

## Compilación:

### Compilar
`make`

### Ejecutar benchmarks
`make benchmark`

### Ejecutar an ́alisis
`make analysis`

### Pruebas unitarias e integraci ́on
`make test`

### Limpiar
`make clean`

# Sobrecarga
## calculateMetricsFirstPrivate()
Se reemplaza por analyzePerformanceFirstPrivate, esto para calcular la desviacion estandard en MetricsCalculator.
Se utiliza first private para poder calcular este valor de manera paralela y ademas asegurar que los valores promedio de los tiempos se entreguen correctamente y que para cada hebra tenga su propia copia local a la hora de ejecutar los calculos
Por defecto se usa la version normal de analyzePerformance, para usarla, cambiar el valor por defecto del metodo runScalabilityTest de mode a 1 en Benchmark.h.
ejemplo:
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
        bool perform_diagnostics = false, // Nuevo: Controla si se ejecuta la fase de diagnóstico
        int mode = 1
    );

