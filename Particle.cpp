#include "Particle.h"

Particle::Particle(double m, double x, double y){
    this->mass = m;
    this->x = x;
    this->y = y;
    this->vx = 0.0;
    this->vy = 0.0;
    this->ax = 0.0;
    this->ay = 0.0;
}

double Particle::getMass() const {
    return mass;
}

double Particle::getX() const {
    return x;
}

double Particle::getY() const {
    return y;
}

double Particle::getVx() const {
    return vx;
}

double Particle::getVy() const {
    return vy;
}

double Particle::getAx() const {
    return ax;
}

double Particle::getAy() const {
    return ay;
}