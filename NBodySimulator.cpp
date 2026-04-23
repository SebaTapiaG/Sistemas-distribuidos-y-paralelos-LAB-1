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

void NBodySimulator::integrateEuler(int sync_type){
        switch(sync_type){
        auto &bodies = system->getBodies();

        //Atomic
        case 0:
            #pragma omp parallel for
            for(int i = 0; i <= bodies.size(); i++){
                double newVx = bodies[i].getVx() + bodies[i].getAx() * time_step;
                double newVy = bodies[i].getVy() + bodies[i].getAy() * time_step;
                double newX = bodies[i].getX() + newVx * time_step;
                double newY = bodies[i].getY() + newVy * time_step;

                #pragma omp atomic
                bodies[i].setVx(newVx);
                
                #pragma omp atomic
                bodies[i].setVy(newVy);

                #pragma omp atomic
                bodies[i].setX(newX);

                #pragma omp atomic
                bodies[i].setY(newY);
            }
            break;
        
        //Critical
        case 1:
            #pragma omp parallel for
            for(int i = 0; i <= bodies.size(); i++){
                double newVx = bodies[i].getVx() + bodies[i].getAx() * time_step;
                double newVy = bodies[i].getVy() + bodies[i].getAy() * time_step;
                double newX = bodies[i].getX() + newVx * time_step;
                double newY = bodies[i].getY() + newVy * time_step;

                #pragma omp critical
                {
                    bodies[i].setVx(newVx);
                    bodies[i].setVy(newVy);
                    bodies[i].setX(newX);
                    bodies[i].setY(newY);
                }
            }
            break;

        //Nowait
        case 2:
            break;

        default:
            throw std::invalid_argument("sync_type solo puede ser 0, 1 o 2.");
            break;
    }

}

void NBodySimulator::integrateEuler(int sync_type, bool use_barrier){
    switch(use_barrier){
        case false:
            integrateEuler(sync_type);
        case true:
            break;
            
        default: 
            throw std::invalid_argument("use_barrier solo puede ser 0, 1.");
            break;
    }
}

// ── Calcula energía cinética y potencial

void NBodySimulator::calculateEnergy() {
    double k = 0;
    double u = 0;
    vector<Particle> &bodies = system->getBodies();

    for (int i = 0; i <= bodies.size(); i++) {
        k += bodies[i].getMass() + 
             (pow(bodies[i].getVx(), 2) + 
             pow(bodies[i].getVy(), 2));

        for (int j = 0; j < i; j++) {
            u += (bodies[i].getMass() * bodies[j].getMass()) /
                 sqrt(
                    pow(abs(
                        sqrt(pow(bodies[j].getX(), 2) + pow(bodies[j].getY(), 2)) - 
                        sqrt(pow(bodies[i].getX(), 2) + pow(bodies[i].getY(), 2)))
                    , 2) + pow(system->getEpsilon(), 2)
                 );
        }
    }
    u = -1 * system->getG() * u;
    k = k / 2; // Falta ver qué hacemos con estos valores
}

void NBodySimulator::calculateEnergy(int method){
    switch(method){
        case 0:
            break;

        case 1:
            break;

        default:
            throw std::invalid_argument("method solo puede ser 0 o 1.");
            break;
    }
}

void NBodySimulator::calculateEnergy(int method, bool use_private){
    switch(use_private){
        case 0:
            calculateEnergy(method);
            break;

        case 1:
            break;

        default: 
            throw std::invalid_argument("use_private solo puede ser 0, 1.");
            break;
    }
}

// ── 

void NBodySimulator::processBodies(){

}

void NBodySimulator::processBodies(int task_type){

}

void NBodySimulator::processBodies(int task_type, bool use_single){

}

// ──

void NBodySimulator::simulatePhasesBarrier(){

}

// ──

void NBodySimulator::parallelInitializationSingle(){

}