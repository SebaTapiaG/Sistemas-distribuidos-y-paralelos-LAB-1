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
    if(use_barrier){

    }
    else{
        integrateEuler(sync_type);
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

    // Bucle temporal serial
    for(int i = 0; i < iter; i++) {
        
        if (task_type == 0) {
            // task: Usando tareas explícitas de OpenMP
            #pragma omp parallel
            {
                #pragma omp single
                {
                    #pragma omp task
                    system->computeAccelerations();
                    
                    #pragma omp taskwait // Esperamos que se calculen las fuerzas

                    #pragma omp task
                    integrateEuler(); // Aquí integramos (asumiendo que ya paraleliza internamente o no)
                }
            }
        } else if (task_type == 1) {
            // parallel_for: El flujo clásico que ya tienes implementado
            system->computeAccelerations();
            integrateEuler(1); // Usando un tipo de sincronización, ej: critical
        }

        // Calculamos energía (usando reduction por defecto para mayor velocidad)
        auto [ui, ki] = calculateEnergy(0); 
        data.u[i] = ui;
        data.k[i] = ki;
        data.bodies[i] = system->getBodies();
    }
    return data;
}

// ── Funciones demostrativas para mostrar el uso de barreras y single
void NBodySimulator::simulatePhasesBarrier() {
    #pragma omp parallel
    {
        // Fase 1: Todos calculan algo
        system->computeAccelerations();
        
        // Nadie avanza a mover las partículas hasta que TODOS terminen de calcular fuerzas
        #pragma omp barrier 
        
        // Fase 2: Mover partículas
        #pragma omp for
        for(size_t i = 0; i < system->getBodies().size(); ++i) {
            auto& bodies = system->getBodies();
            bodies[i].setVx(bodies[i].getVx() + bodies[i].getAx() * time_step);
            bodies[i].setVy(bodies[i].getVy() + bodies[i].getAy() * time_step);
            bodies[i].setX(bodies[i].getX() + bodies[i].getVx() * time_step);
            bodies[i].setY(bodies[i].getY() + bodies[i].getVy() * time_step);
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