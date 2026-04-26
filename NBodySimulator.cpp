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
            // Atomic write no funciona con setters, así que por ahora lo dejo vacío
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

    for (auto i = 0; i <= bodies.size(); i++) {
        k += bodies[i].getMass() + 
             (pow(bodies[i].getVx(), 2) + 
             pow(bodies[i].getVy(), 2));

        for (int j = 0; j < i; j++) {
            u += (bodies[i].getMass() * bodies[j].getMass()) /
                 sqrt(
                     pow(abs(
                         sqrt(pow(bodies[j].getX(), 2) + pow(bodies[j].getY(), 2)) - 
                         sqrt(pow(bodies[i].getX(), 2) + pow(bodies[i].getY(), 2))
                     ), 2) + pow(system->getEpsilon(), 2)
                 );
        }
    }
    u = -1 * system->getG() * u;
    k = k / 2;
    return {u, k};
}

void NBodySimulator::calculateEnergy(int method){
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
}

void NBodySimulator::calculateEnergy(int method, bool use_private){
    if(use_private){

    }
    else{
        calculateEnergy(method);
    }
}

// ── 

void NBodySimulator::processBodies(int iter){
    vector<double> k, u;
    for(int i=0; i<iter; i++){
        system->computeAccelerations();
        integrateEuler();
        auto [ui, ki] = calculateEnergy();
        u[i] = ui;
        k[i] = ki;
    }
}

void NBodySimulator::processBodies(int iter, int task_type){

}

void NBodySimulator::processBodies(int iter, int task_type, bool use_single){
    if(use_single){

    }
    else{
        processBodies(task_type);
    }
}

// ──

void NBodySimulator::simulatePhasesBarrier(){

}

// ──

void NBodySimulator::parallelInitializationSingle(){

}