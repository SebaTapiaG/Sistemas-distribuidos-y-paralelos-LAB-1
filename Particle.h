#ifndef PARTICLE_H
#define PARTICLE_H

using namespace std;

/**
 * Particle
 * 
 * Entidad física puntual en espcaio 2D.
 * Proporciona métodos para el moviemiento de estas.
*/
class Particle {
    private:
        double mass; //Masa
        double x, y; //Posiciones
        double vx, vy; //Velocidades
        double ax, ay; //Aceleraciones

    public:
        Particle(double m, double x, double y);
        Particle(double m, double x, double y, double vx, double vy);
        void setAcceleration(double ax_, double ay_);
        void addAcceleration(double dax, double day);
        void kick(double dt); // Actualiza velocidades
        void drift(double dt); // Actualiza posiciones
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
        void setAx(double ax_);
        void setAy(double ay_);

    // ** Pone ax = ay = 0 (llamar antes de acumular contribuciones). */
    void zeroAcceleration();

};

#endif