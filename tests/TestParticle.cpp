#include <gtest/gtest.h>
#include "Particle.h"

TEST(ParticleTest, ConstructorInicializaEstado) {
    Particle p(10.5, 2.0, -3.0);

    EXPECT_DOUBLE_EQ(p.getMass(), 10.5);
    EXPECT_DOUBLE_EQ(p.getX(), 2.0);
    EXPECT_DOUBLE_EQ(p.getY(), -3.0);
    EXPECT_DOUBLE_EQ(p.getVx(), 0.0);
    EXPECT_DOUBLE_EQ(p.getVy(), 0.0);
    EXPECT_DOUBLE_EQ(p.getAx(), 0.0);
    EXPECT_DOUBLE_EQ(p.getAy(), 0.0);
}

TEST(ParticleTest, SetAndAddAcceleration) {
    Particle p(1.0, 0.0, 0.0);

    p.setAcceleration(1.5, -0.5);
    EXPECT_DOUBLE_EQ(p.getAx(), 1.5);
    EXPECT_DOUBLE_EQ(p.getAy(), -0.5);

    p.addAcceleration(0.5, 2.0);
    EXPECT_DOUBLE_EQ(p.getAx(), 2.0);
    EXPECT_DOUBLE_EQ(p.getAy(), 1.5);
}

TEST(ParticleTest, ZeroAccelerationResetsValues) {
    Particle p(1.0, 0.0, 0.0);
    p.setAcceleration(9.0, -7.0);

    p.zeroAcceleration();

    EXPECT_DOUBLE_EQ(p.getAx(), 0.0);
    EXPECT_DOUBLE_EQ(p.getAy(), 0.0);
}

TEST(ParticleTest, KickUpdatesVelocity) {
    Particle p(1.0, 0.0, 0.0);
    p.setAcceleration(2.0, -4.0);

    p.kick(0.5);

    EXPECT_DOUBLE_EQ(p.getVx(), 1.0);
    EXPECT_DOUBLE_EQ(p.getVy(), -2.0);
}

TEST(ParticleTest, DriftUpdatesPosition) {
    Particle p(1.0, 1.0, 2.0);
    p.setVx(3.0);
    p.setVy(-1.0);

    p.drift(2.0);

    EXPECT_DOUBLE_EQ(p.getX(), 7.0);
    EXPECT_DOUBLE_EQ(p.getY(), 0.0);
}