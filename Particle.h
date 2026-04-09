#ifndef PARTICLE_H
#define PARTICLE_H

using namespace std;

class Particle {
    private:
        double mass;
        double x, y, vx, vy, ax, ay;

    public:
        Particle(double m, double x, double y);
        void setAcceleration(double ax_, double ay_);
        void addAcceleration(double dax, double day);
        void kick(double dt); // v += a*dt
        void drift(double dt); // r += v*dt
        double getMass() const;

    // Getters
        double getX() const;
        double getY() const;
        double getVx() const;
        double getVy() const;
        double getAx() const;
        double getAy() const;

    // Setters
        void setX(double x_);
        void setY(double y_);
        void setVx(double vx_);
        void setVy(double vy_);

};

#endif