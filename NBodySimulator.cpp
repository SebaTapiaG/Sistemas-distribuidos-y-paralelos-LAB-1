#include "NBodySimulator.h"
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

    // NO PARALELIZAR ESTE FOR
    for(int i = 0; i<iter; i++){
        system->computeAccelerations();
        auto [ui, ki] = calculateEnergy();
        integrateEuler();
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
                    system->computeAccelerations(schedule_type, chunk_size);
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