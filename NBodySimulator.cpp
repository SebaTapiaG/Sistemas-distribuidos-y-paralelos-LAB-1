#include "NBodySimulator.h"
#include "kernels/energy.cuh"
#include "kernels/integration.cuh"
#include <omp.h>
#include <cmath>
#include <stdexcept>

// ── Constructor ──────────────────────────────────────────────────────────────

NBodySimulator::NBodySimulator(NBodySystem *system, double time_step)
: system(system), time_step(time_step){
    if(time_step<=0)
        throw std::invalid_argument("time_step debe ser mayor a 0.");
} 

// ── Calcula las nuevas velocidades y posiciones según las aceleraciones
void NBodySimulator::integrateEuler(){
    system->computeAccelerations();
    for(auto &particle : system->getBodies()){

        particle.kick(time_step);
        particle.drift(time_step);
    }
}

// ── 1. Critical 2. Nowait
void NBodySimulator::integrateEuler(int sync_type){
    auto& bodies = system->getBodies();
    switch(sync_type){

        // atomic
        case 0:
        integrateEuler();
            break;

        // critical
        case 1:
            #pragma omp parallel for
            for(size_t i = 0; i < bodies.size(); ++i) {
                #pragma omp critical
                {
                    bodies[i].kick(time_step);
                    bodies[i].drift(time_step);
                }
            }
            break;

        // nowait
        case 2:
            #pragma omp parallel
            {
                #pragma omp for nowait
                for(size_t i = 0; i < bodies.size(); ++i) {
                    bodies[i].kick(time_step);
                    bodies[i].drift(time_step);
                }
            }
            break;

        default:
            throw std::invalid_argument("sync_type solo puede ser 0, 1 o 2.");
            break;
    }
}

void NBodySimulator::integrateEuler(int sync_type, bool use_barrier){
    integrateEuler(sync_type); // Primero movemos las partículas
    
    if(use_barrier){
        #pragma omp parallel
        {
        // Si estamos dentro de una región paralela, forzamos la barrera
        #pragma omp barrier 
        }
    }
}

// ── Demostración de la cláusula 'private' y Reducción Manual ─────────────────
pair<double, double> NBodySimulator::calculateEnergy(int method, bool use_private) {
    if (!use_private) {
        return calculateEnergy(method);
    }

    double k = 0.0;
    double u = 0.0;
    vector<Particle>& bodies = system->getBodies();

    // Declaramos variables AFUERA. La cláusula 'private' las aislará luego.
    size_t j;
    double dx, dy, distSq;

    if (method == 0) {
        // method 0: Reduction (Optimización nativa de OpenMP)
        #pragma omp parallel for reduction(+:k, u) private(j, dx, dy, distSq) schedule(dynamic)
        for (size_t i = 0; i < bodies.size(); i++) {
            k += bodies[i].getMass() * (pow(bodies[i].getVx(), 2) + pow(bodies[i].getVy(), 2));
            
            for (j = 0; j < i; j++) {
                dx = bodies[j].getX() - bodies[i].getX();
                dy = bodies[j].getY() - bodies[i].getY();
                distSq = (dx * dx) + (dy * dy) + pow(system->getEpsilon(), 2);
                u += (bodies[i].getMass() * bodies[j].getMass()) / sqrt(distSq);
            }
        }
    } else if (method == 1) {
        // method 1: Atomic (Optimizado con Reducción Manual)
        double local_k = 0.0;
        double local_u = 0.0;

        // firstprivate: Da a cada hilo su propia copia de local_k y local_u valiendo 0.0
        // private: Aísla las variables de cálculo iterativo
        #pragma omp parallel firstprivate(local_k, local_u) private(j, dx, dy, distSq)
        {
            #pragma omp for schedule(dynamic)
            for (size_t i = 0; i < bodies.size(); i++) {
                // Sumamos sin usar atomic aquí, cada hilo suma en su copia privada
                local_k += bodies[i].getMass() * (pow(bodies[i].getVx(), 2) + pow(bodies[i].getVy(), 2));
                
                for (j = 0; j < i; j++) {
                    dx = bodies[j].getX() - bodies[i].getX();
                    dy = bodies[j].getY() - bodies[i].getY();
                    distSq = (dx * dx) + (dy * dy) + pow(system->getEpsilon(), 2);
                    local_u += (bodies[i].getMass() * bodies[j].getMass()) / sqrt(distSq);
                }
            }

            // Fuera del for, pero dentro del parallel:
            // Los hilos suman su sub-total a la variable compartida UNA sola vez por hilo.
            #pragma omp atomic
            k += local_k;
            #pragma omp atomic
            u += local_u;
        }
    } else {
        throw std::invalid_argument("method solo puede ser 0 (reduction) o 1 (atomic).");
    }

    u = -1.0 * system->getG() * u;
    k = k * 0.5;
    return {u, k};
}

// ── Calcula energía cinética y potencial
pair<double, double> NBodySimulator::calculateEnergy() {
    double k = 0;
    double u = 0;
    vector<Particle>& bodies = system->getBodies();

    for (size_t i = 0; i < bodies.size(); i++) {
        k += bodies[i].getMass() * 
             (pow(bodies[i].getVx(), 2) + 
             pow(bodies[i].getVy(), 2));

        for (size_t j = 0; j < i; j++) {
            double dx = bodies[j].getX() - bodies[i].getX();
            double dy = bodies[j].getY() - bodies[i].getY();
            double distSq = (dx * dx) + (dy * dy) + pow(system->getEpsilon(), 2);
            u += (bodies[i].getMass() * bodies[j].getMass()) / sqrt(distSq);
        }
    }
    u = -1 * system->getG() * u;
    k = k * 0.5;
    return {u, k};
}

// ── Calcula energía cinética y potencial en paralelo
pair<double, double> NBodySimulator::calculateEnergy(int method) {
    double k = 0.0;
    double u = 0.0;
    vector<Particle>& bodies = system->getBodies();

    switch(method) {
        // 0: Usando reduction (La forma más eficiente en OpenMP)
        case 0:
            #pragma omp parallel for reduction(+:k, u) schedule(dynamic)
            for (size_t i = 0; i < bodies.size(); i++) {
                k += bodies[i].getMass() * (pow(bodies[i].getVx(), 2) + pow(bodies[i].getVy(), 2));
                
                for (size_t j = 0; j < i; j++) {
                    double dx = bodies[j].getX() - bodies[i].getX();
                    double dy = bodies[j].getY() - bodies[i].getY();
                    double distSq = (dx * dx) + (dy * dy) + pow(system->getEpsilon(), 2);
                    u += (bodies[i].getMass() * bodies[j].getMass()) / sqrt(distSq);
                }
            }
            break;

        // 1: Usando atomic (Menos eficiente, obliga a los hilos a esperar para sumar)
        case 1:
            #pragma omp parallel for schedule(dynamic)
            for (size_t i = 0; i < bodies.size(); i++) {
                double local_k = bodies[i].getMass() * (pow(bodies[i].getVx(), 2) + pow(bodies[i].getVy(), 2));
                double local_u = 0.0;
                
                for (size_t j = 0; j < i; j++) {
                    double dx = bodies[j].getX() - bodies[i].getX();
                    double dy = bodies[j].getY() - bodies[i].getY();
                    double distSq = (dx * dx) + (dy * dy) + pow(system->getEpsilon(), 2);
                    local_u += (bodies[i].getMass() * bodies[j].getMass()) / sqrt(distSq);
                }

                // Protegemos la suma en las variables compartidas
                #pragma omp atomic
                k += local_k;
                #pragma omp atomic
                u += local_u;
            }
            break;

        default:
            throw std::invalid_argument("method solo puede ser 0 (reduction) o 1 (atomic).");
    }

    u = -1.0 * system->getG() * u;
    k = k * 0.5;
    return {u, k};
}

// Versión serial
simulation_data NBodySimulator::processBodies(int iter){
    simulation_data data;
    data.u.resize(iter);
    data.k.resize(iter);
    data.bodies.resize(iter);

    for(int i = 0; i<iter; i++){
        system->computeAccelerations();
        integrateEuler();
        auto [ui, ki] = calculateEnergy();
        data.u[i] = ui;
        data.k[i] = ki;
        data.bodies[i] = system->getBodies();
    }
    return data;
}

// ── processBodies usando paralelización exterior (Tasks vs Parallel For)
simulation_data NBodySimulator::processBodies(int iter, int task_type, int sync_type, 
    int method, int schedule_type, int chunk_size) {
    simulation_data data;
    data.u.resize(iter);
    data.k.resize(iter);
    data.bodies.resize(iter);
 
    for (int i = 0; i < iter; i++) {
 
        if (task_type == 0) {
            // ── Enfoque con tasks ─────────────────────────────────────────
            // Fase 1: calcular aceleraciones (debe completarse antes de mover)
            // Se hace en single para que solo un hilo lo ejecute; los demas
            // esperan en la barrera implicita al final del single.
            #pragma omp parallel
            {
                #pragma omp single
                {
                    system->computeAccelerations();
                }
                // Barrera implicita del single: todos los hilos tienen las
                // aceleraciones actualizadas antes de continuar.
 
                // Fase 2: kick + drift con una tarea por cuerpo
                #pragma omp single
                {
                    const int N = system->getBodies().size();
                    for (int k = 0; k < N; ++k) {
                        // Compartimos el puntero system y copiamos el índice k
                        #pragma omp task shared(system) firstprivate(k)
                        {
                            // Accedemos directamente al vector original en memoria
                            system->getBodies()[k].kick(time_step);
                            system->getBodies()[k].drift(time_step);
                        }
                    }
                    #pragma omp taskwait
                }
            }
 
        } else {
            system->computeAccelerations(schedule_type, chunk_size);
            integrateEuler(sync_type);
        }
 
        // Energia: reduction (metodo 0) es el mas eficiente para sumas globales
        auto [ui, ki] = calculateEnergy(method);
        data.u[i] = ui;
        data.k[i] = ki;
        data.bodies[i] = system->getBodies();
    }
    return data;
}

// ── Sobrecarga processBodies para contrastar single vs single nowait ─────────
simulation_data NBodySimulator::processBodies(int iter, int task_type, bool use_single, int sync_type, 
    int method, int schedule_type, int chunk_size, int use_private, int use_barrier) {
    simulation_data data;
    data.u.resize(iter);
    data.k.resize(iter);
    data.bodies.resize(iter);

    for (int i = 0; i < iter; i++) {

        if (task_type == 0) {
            // Punteros y variables locales para ser capturadas de forma segura por firstprivate
            NBodySystem* sys_ptr = system;
            double dt = time_step;

            #pragma omp parallel
            {
                // Fase 1: Cálculo de fuerzas contrastando barrera implícita vs explícita
                if (use_single) {
                    // Semántica 1: 'single' normal. 
                    // OpenMP añade una barrera implícita invisible justo al cerrar la llave.
                    #pragma omp single
                    {
                        sys_ptr->computeAccelerations();
                    } 
                } else {
                    // Semántica 2: 'single nowait' + 'barrier' explícita.
                    // El nowait quita la barrera implícita, delegando la responsabilidad al programador.
                    #pragma omp single nowait
                    {
                        sys_ptr->computeAccelerations();
                    }
                    #pragma omp barrier
                }

                // Fase 2: Integración con una tarea por partícula
                #pragma omp single
                {
                    const int M = sys_ptr->getBodies().size();
                    for (int k = 0; k < M; ++k) {
                        // firstprivate garantiza que el hilo que ejecuta la tarea tenga
                        // una copia local e inmutable del índice 'k' y los punteros en ese instante.
                        #pragma omp task firstprivate(k, sys_ptr, dt)
                        {
                            sys_ptr->getBodies()[k].kick(dt);
                            sys_ptr->getBodies()[k].drift(dt);
                        }
                    }
                    // taskwait asegura que todas las tareas terminen antes de pasar a la siguiente iteración
                    #pragma omp taskwait
                }
            }
        } else {
            // Si no es task_type 0, usamos la lógica paralela estándar
            system->computeAccelerations(schedule_type, chunk_size); 
            integrateEuler(sync_type, use_barrier);
        }

        // Medición de energía con reducción nativa y almacenamiento de la "foto" del instante
        auto [ui, ki] = calculateEnergy(method, use_private);
        data.u[i] = ui;
        data.k[i] = ki;
        data.bodies[i] = system->getBodies();
    }
    
    return data;
}

// ── Funciones demostrativas para mostrar el uso de barreras y single
void NBodySimulator::simulatePhasesBarrier() {
    #pragma omp parallel default(none) shared(system, time_step)
    {
        // Fase 1: solo un hilo calcula todas las aceleraciones
        #pragma omp single nowait  // nowait: no barrera implicita aqui,
        {                          // la ponemos nosotros de forma explicita abajo
            system->computeAccelerations();
        }
 
        // Barrera EXPLICITA: ningun hilo avanza a mover particulas
        // hasta que computeAccelerations() haya terminado completamente.
        #pragma omp barrier
 
        // Fase 2: todos los hilos colaboran en kick + drift
        auto& bodies = system->getBodies();
        const int N  = static_cast<int>(bodies.size());
 
        #pragma omp for schedule(static)
        for (int i = 0; i < N; ++i) {
            bodies[i].kick(time_step);
            bodies[i].drift(time_step);
        }
    }
}

void NBodySimulator::parallelInitializationSingle() {
    #pragma omp parallel
    {
        // Solo un hilo hará esta inicialización, los demás esperarán al final del bloque single
        #pragma omp single
        {
            // Inicialización dummy de ejemplo
            system->setG(1.0);
            system->setEpsilon(0.1);
        }
        // Aquí hay una barrera implícita: los demás hilos esperan a que el hilo 'single' termine.
    }
}


// Utiliza una variable inicializada antes de la región paralela y entrega 
// una copia a cada hilo con ese valor inicial.
double NBodySimulator::calculateMetricsFirstprivate() {
    // Valor base que queremos que todos los hilos tengan al iniciar su trabajo
    double factor_inicial = 10.0; 
    double metrica_total = 0.0;
    const auto& bodies = system->getBodies();

    // firstprivate asegura que cada hilo empiece con su propia copia de 
    // 'factor_inicial' valiendo exactamente 10.0
    #pragma omp parallel for firstprivate(factor_inicial) reduction(+:metrica_total)
    for (size_t i = 0; i < bodies.size(); ++i) {
        // Cada hilo modifica su propia copia de factor_inicial sin afectar a los demás
        factor_inicial += bodies[i].getMass(); 
        
        // Sumamos a una métrica global
        metrica_total += factor_inicial;
    }
    
    return metrica_total;
}

// ── Demostración de lastprivate ──────────────────────────────────────────────
// Obtiene el valor de una variable tal como quedó en la ÚLTIMA iteración 
// lógica del ciclo (i == N - 1), llevándola al ámbito externo.
double NBodySimulator::calculateFinalStateLastprivate() {
    double ultimo_x = -1.0; 
    const auto& bodies = system->getBodies();

    // lastprivate asegura que, al terminar el parallel for, 'ultimo_x' 
    // conserve el valor que se le asignó en la iteración i = bodies.size() - 1
    #pragma omp parallel for lastprivate(ultimo_x)
    for (size_t i = 0; i < bodies.size(); ++i) {
        ultimo_x = bodies[i].getX();
    }
    
    // Al terminar, ultimo_x es garantizadamente la posición X de la última 
    // partícula procesada, sin importar qué hilo ejecutó esa última iteración.
    return ultimo_x;
}

// ── Integración Euler en GPU (Aceleración + Kick/Drift) ──────────────────────
void NBodySimulator::integrateEulerGpu(int variant, int block_size) {
    const int N = static_cast<int>(system->getBodies().size());
    if (N == 0) return;

    // 1. Lanzar cálculo de aceleraciones en GPU (escribe en d_ax, d_ay)
    system->computeAccelerationsGpu(variant, block_size);

    // 2. Lanzar Kernel de Integración Euler (Kick & Drift) sobre memoria VRAM
    launchEulerIntegrationGpu(
        system->getGpuX(),
        system->getGpuY(),
        system->getGpuVx(),
        system->getGpuVy(),
        system->getGpuAx(),
        system->getGpuAy(),
        N,
        time_step,
        block_size
    );
}
void NBodySimulator::calculateEnergyGpu(int method, int block_size, double* d_u_out, double* d_k_out) {
    const int N = static_cast<int>(system->getBodies().size());
    if (N <= 0) return;

    launchComputeEnergyGpu(
        system->getGpuX(),
        system->getGpuY(),
        system->getGpuVx(),
        system->getGpuVy(),
        system->getGpuMass(),
        N,
        system->getG(),
        system->getEpsilon(),
        method,
        block_size,
        d_u_out,
        d_k_out
    );
}
//temporalmente reemplazada por la version con grabacion de fotogramas opcional
/*
// ── Pipeline de Simulación en GPU (Guardando fotogramas e historia de energía)
simulation_data NBodySimulator::processBodiesGpu(int iter, int variant, int energy_method, int block_size) {
    simulation_data data;
    data.u.resize(iter);
    data.k.resize(iter);
    data.bodies.resize(iter); // Guardará los 'iter' fotogramas del sistema

    const int N = static_cast<int>(system->getBodies().size());
    if (N == 0) return data;

    // 1. Preparación de memoria e inicialización en GPU
    system->convertAosToSoa();
    system->allocateGpuMemory();
    system->copyHostToDevice();

    // Reservar historia de energía en VRAM (CudaBuffer RAII)
    CudaBuffer<double> d_u_vec(iter);
    CudaBuffer<double> d_k_vec(iter);

    // 2. Bucle principal de simulación
    for (int i = 0; i < iter; ++i) {
        // A) Integración física en GPU
        integrateEulerGpu(variant, block_size);

        // B) Cálculo de energía en GPU (0 sincronizaciones CPU-GPU, escribe en VRAM)
        calculateEnergyGpu(energy_method, block_size, d_u_vec.get() + i, d_k_vec.get() + i);

        // C) Descargar y guardar el fotograma actual en RAM para la iteración 'i'
        system->copyDeviceToHost();
        system->convertSoaToAos();
        data.bodies[i] = system->getBodies();
    }

    // 3. UNA SOLA transferencia final masiva (D2H) para toda la historia de energías
    d_u_vec.ToHost(data.u.data());
    d_k_vec.ToHost(data.k.data());

    // La memoria de d_u_vec y d_k_vec en VRAM se libera automáticamente aquí al salir
    return data;
}
*/
// ── Paso de simulación completo en GPU (Integración + Energía) ─────────────
void NBodySimulator::stepEulerGpu(int variant, int energy_method, int block_size, double* d_u_ptr, double* d_k_ptr) {
    // 1. Cálculo de aceleración e integración física
    integrateEulerGpu(variant, block_size);

    // 2. Cálculo de energía (si se pasaron punteros válidos en VRAM)
    if (d_u_ptr && d_k_ptr) {
        calculateEnergyGpu(energy_method, block_size, d_u_ptr, d_k_ptr);
    }
}
//sobrecarga de processBodiesGpu con parámetro opcional para grabar fotogramas
simulation_data NBodySimulator::processBodiesGpu(
    int iter, 
    int variant, 
    int energy_method, 
    int block_size, 
    bool record_frames)
{
    simulation_data data;
    data.u.resize(iter);
    data.k.resize(iter);
    if (record_frames) {
        data.bodies.resize(iter);
    }

    const int N = static_cast<int>(system->getBodies().size());
    if (N == 0) return data;

    // 1. Preparación de memoria (H2D) fuera del loop
    system->convertAosToSoa();
    system->allocateGpuMemory();
    system->copyHostToDevice();

    // Buffers temporales en VRAM para la historia de energía
    CudaBuffer<double> d_u_vec(iter);
    CudaBuffer<double> d_k_vec(iter);

    // 2. Bucle principal
    for (int i = 0; i < iter; ++i) {
        // A y B) Cómputo 100% en GPU
        stepEulerGpu(variant, energy_method, block_size, d_u_vec.get() + i, d_k_vec.get() + i);

        // C) Descarga Opcional: Solo si explícitamente se pidieron fotogramas
        if (record_frames) {
            system->copyDeviceToHost();
            system->convertSoaToAos();
            data.bodies[i] = system->getBodies();
        }
    }

    // 3. Descargar el estado FINAL del sistema a Host (sólo una vez al terminar)
    if (!record_frames) {
        system->copyDeviceToHost();
        system->convertSoaToAos();
    }

    // 4. Descarga masiva (D2H) de todas las energías
    d_u_vec.ToHost(data.u.data());
    d_k_vec.ToHost(data.k.data());

    return data;
}