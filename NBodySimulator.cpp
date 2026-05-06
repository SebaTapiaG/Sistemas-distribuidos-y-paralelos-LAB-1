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

        particle.setVx(particle.getVx()+particle.getAx()*time_step);
        particle.setVy(particle.getVy()+particle.getAy()*time_step);

        particle.setX(particle.getX()+particle.getVx()*time_step);
        particle.setY(particle.getY()+particle.getVy()*time_step);
    }
}

// ── 0. Atomic 1. Critical 2. Nowait
void NBodySimulator::integrateEuler(int sync_type){
    auto& bodies = system->getBodies();
    switch(sync_type){

        // atomic
        case 0:
        /*
            #pragma omp parallel for
            for(size_t i = 0; i < bodies.size(); ++i) {
                double new_vx = bodies[i].getVx() + bodies[i].getAx() * time_step;
                double new_vy = bodies[i].getVy() + bodies[i].getAy() * time_step;
                double new_x = bodies[i].getX() + new_vx * time_step;
                double new_y = bodies[i].getY() + new_vy * time_step;

                Cambiar esto, porque atomic write no acepta setters

                #pragma omp atomic write
                bodies[i].setVx(new_vx);
                #pragma omp atomic write
                bodies[i].setVy(new_vy);
                #pragma omp atomic write 
                bodies[i].setX(new_x);
                #pragma omp atomic write
                bodies[i].setY(new_y);
            }*/
            break;

        // critical
        case 1:
            #pragma omp parallel for
            for(size_t i = 0; i < bodies.size(); ++i) {
                double new_vx = bodies[i].getVx() + bodies[i].getAx() * time_step;
                double new_vy = bodies[i].getVy() + bodies[i].getAy() * time_step;
                double new_x = bodies[i].getX() + new_vx * time_step;
                double new_y = bodies[i].getY() + new_vy * time_step;

                #pragma omp critical
                {
                    bodies[i].setVx(new_vx);
                    bodies[i].setVy(new_vy);
                    bodies[i].setX(new_x);
                    bodies[i].setY(new_y);
                }
            }
            break;

        // nowait
        case 2:
            #pragma omp parallel
            {
                #pragma omp for nowait
                for(size_t i = 0; i < bodies.size(); ++i) {
                    double vx = bodies[i].getVx() + bodies[i].getAx() * time_step;
                    double vy = bodies[i].getVy() + bodies[i].getAy() * time_step;
                    
                    bodies[i].setVx(vx);
                    bodies[i].setVy(vy);
                    bodies[i].setX(bodies[i].getX() + vx * time_step);
                    bodies[i].setY(bodies[i].getY() + vy * time_step);
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
        // Si estamos dentro de una región paralela, forzamos la barrera
        #pragma omp barrier 
    }
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

/*pair<double, double> NBodySimulator::calculateEnergy(int method){
    switch(method){
        case 0:
            calculateEnergy();
            break;

        case 1:
            break;

        default:
            throw std::invalid_argument("method solo puede ser 0 o 1.");
            break;
    }
}*/

/*
pair<double, double> NBodySimulator::calculateEnergy(int method, bool use_private){
    if(use_private){

    }
    else{
        calculateEnergy(method);
    }
}*/

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

// ── processBodies usando paralelización exterior (Tasks vs Parallel For)
simulation_data NBodySimulator::processBodies(int iter, int task_type) {
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
                    // computeAccelerations() tiene su propio parallel for interno
                    // => no lanzar como task, llamar directamente desde single
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
            // ── Enfoque con parallel for (task_type == 1) ─────────────────
            system->computeAccelerations(0);
            
            // Usamos la versión paralela nowait (2) o critical (1)
            integrateEuler(2);
        }
 
        // Energia: reduction (metodo 0) es el mas eficiente para sumas globales
        auto [ui, ki] = calculateEnergy(0);
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
            // kick: v += a * dt
            bodies[i].setVx(bodies[i].getVx() + bodies[i].getAx() * time_step);
            bodies[i].setVy(bodies[i].getVy() + bodies[i].getAy() * time_step);
            // drift: r += v * dt  (usa la velocidad YA actualizada — Euler)
            bodies[i].setX(bodies[i].getX() + bodies[i].getVx() * time_step);
            bodies[i].setY(bodies[i].getY() + bodies[i].getVy() * time_step);
        }
        // Barrera implicita al final del #pragma omp for
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