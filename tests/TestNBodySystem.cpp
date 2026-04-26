#include <cmath>
#include <gtest/gtest.h>
#include "NBodySystem.h"

namespace {

constexpr double kTolerance = 1e-9;

NBodySystem CreateSmallSystem() {
    NBodySystem system(1.0, 0.1);
    system.addParticle(Particle(1.0, 0.0, 0.0));
    system.addParticle(Particle(2.0, 1.0, 0.5));
    system.addParticle(Particle(0.5, -1.0, -1.0));
    return system;
}

struct ExpectedAcceleration {
    double ax;
    double ay;
};

ExpectedAcceleration ComputeExpectedAcceleration(const NBodySystem& system, size_t index) {
    const auto& bodies = system.getBodies();
    const double epsilon = system.getEpsilon();
    const double epsilon2 = epsilon * epsilon;

    double ax = 0.0;
    double ay = 0.0;
    const double xi = bodies[index].getX();
    const double yi = bodies[index].getY();

    for (size_t j = 0; j < system.getBodies().size(); ++j) {
        if (j == index) {
            continue;
        }

        const double dx = bodies[j].getX() - xi;
        const double dy = bodies[j].getY() - yi;
        const double dist2 = dx * dx + dy * dy + epsilon2;
        const double dist3 = dist2 * std::sqrt(dist2);
        const double factor = system.getG() * bodies[j].getMass() / dist3;

        ax += factor * dx;
        ay += factor * dy;
    }

    return {ax, ay};
}

}  // namespace

TEST(NBodySystemTest, ConstructorStoresPhysicalParameters) {
    NBodySystem system(2.5, 0.2);

    EXPECT_DOUBLE_EQ(system.getG(), 2.5);
    EXPECT_DOUBLE_EQ(system.getEpsilon(), 0.2);
    EXPECT_EQ(system.getBodies().size(), 0);
}

TEST(NBodySystemTest, ConstructorRejectsInvalidParameters) {
    EXPECT_THROW(NBodySystem(1.0, 0.0), std::invalid_argument);
    EXPECT_THROW(NBodySystem(1.0, -0.1), std::invalid_argument);
    EXPECT_THROW(NBodySystem(-1.0, 0.1), std::invalid_argument);
}

TEST(NBodySystemTest, AddParticleUpdatesCountAndBodies) {
    NBodySystem system(1.0, 0.1);

    system.addParticle(Particle(3.0, 4.0, -2.0));

    ASSERT_EQ(system.getBodies().size(), 1);
    const auto& bodies = system.getBodies();
    EXPECT_DOUBLE_EQ(bodies[0].getMass(), 3.0);
    EXPECT_DOUBLE_EQ(bodies[0].getX(), 4.0);
    EXPECT_DOUBLE_EQ(bodies[0].getY(), -2.0);
}

TEST(NBodySystemTest, AddParticleRejectsNonPositiveMass) {
    NBodySystem system(1.0, 0.1);

    EXPECT_THROW(system.addParticle(Particle(0.0, 0.0, 0.0)), std::invalid_argument);
    EXPECT_THROW(system.addParticle(Particle(-1.0, 0.0, 0.0)), std::invalid_argument);
}

TEST(NBodySystemTest, ZeroAccelerationsResetsAllBodies) {
    NBodySystem system = CreateSmallSystem();

    system.getBodies()[0].setAcceleration(3.0, -4.0);
    system.getBodies()[1].setAcceleration(-1.0, 2.0);
    system.getBodies()[2].setAcceleration(8.0, 9.0);

    system.zeroAccelerations();

    for (const auto& body : system.getBodies()) {
        EXPECT_DOUBLE_EQ(body.getAx(), 0.0);
        EXPECT_DOUBLE_EQ(body.getAy(), 0.0);
    }
}

TEST(NBodySystemTest, SerialAccelerationMatchesExpectedValues) {
    NBodySystem system = CreateSmallSystem();

    system.computeAccelerations();

    ASSERT_EQ(system.getBodies().size(), 3);
    for (size_t i = 0; i < system.getBodies().size(); ++i) {
        const ExpectedAcceleration expected = ComputeExpectedAcceleration(system, i);
        EXPECT_NEAR(system.getBodies()[i].getAx(), expected.ax, kTolerance) << "Particula " << i;
        EXPECT_NEAR(system.getBodies()[i].getAy(), expected.ay, kTolerance) << "Particula " << i;
    }
}

TEST(NBodySystemTest, ScheduleOverloadMatchesSerialComputation) {
    NBodySystem serial_system = CreateSmallSystem();
    NBodySystem dynamic_system = CreateSmallSystem();
    NBodySystem guided_system = CreateSmallSystem();
    NBodySystem auto_system = CreateSmallSystem();

    serial_system.computeAccelerations();
    dynamic_system.computeAccelerations(1);
    guided_system.computeAccelerations(2);
    auto_system.computeAccelerations(99);

    for (size_t i = 0; i < serial_system.getBodies().size(); ++i) {
        EXPECT_NEAR(serial_system.getBodies()[i].getAx(), dynamic_system.getBodies()[i].getAx(), kTolerance)
            << "Particula " << i;
        EXPECT_NEAR(serial_system.getBodies()[i].getAy(), dynamic_system.getBodies()[i].getAy(), kTolerance)
            << "Particula " << i;

        EXPECT_NEAR(serial_system.getBodies()[i].getAx(), guided_system.getBodies()[i].getAx(), kTolerance)
            << "Particula " << i;
        EXPECT_NEAR(serial_system.getBodies()[i].getAy(), guided_system.getBodies()[i].getAy(), kTolerance)
            << "Particula " << i;

        EXPECT_NEAR(serial_system.getBodies()[i].getAx(), auto_system.getBodies()[i].getAx(), kTolerance)
            << "Particula " << i;
        EXPECT_NEAR(serial_system.getBodies()[i].getAy(), auto_system.getBodies()[i].getAy(), kTolerance)
            << "Particula " << i;
    }
}

TEST(NBodySystemTest, CollapseComputationMatchesSerialComputation) {
    NBodySystem serial_system = CreateSmallSystem();
    NBodySystem collapse_system = CreateSmallSystem();

    serial_system.computeAccelerations();
    collapse_system.computeAccelerationsCollapse();

    ASSERT_EQ(serial_system.getBodies().size(), collapse_system.getBodies().size());
    for (size_t i = 0; i < serial_system.getBodies().size(); ++i) {
        EXPECT_NEAR(serial_system.getBodies()[i].getAx(), collapse_system.getBodies()[i].getAx(), kTolerance)
            << "Particula " << i;
        EXPECT_NEAR(serial_system.getBodies()[i].getAy(), collapse_system.getBodies()[i].getAy(), kTolerance)
            << "Particula " << i;
    }
}