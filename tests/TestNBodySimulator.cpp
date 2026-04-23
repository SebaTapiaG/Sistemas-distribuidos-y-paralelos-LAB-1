#include <gtest/gtest.h>
#include <cmath>
#include <random>

#include "NBodySimulator.h"

namespace {

constexpr int kMainParticleCount = 500;
constexpr unsigned int kMainSeed = 123;
constexpr double kMainG = 1.0;
constexpr double kMainEpsilon = 10.0;

NBodySystem CreateMainLikeSystem() {
	NBodySystem system(kMainG, kMainEpsilon);

	std::mt19937 gen(kMainSeed);
	std::uniform_real_distribution<> random_dis(0.0, 1000.0);
	std::uniform_real_distribution<> random_mass(1.0, 100.0);

	for (int i = 0; i < kMainParticleCount; ++i) {
		double mass = random_mass(gen);
		double x = random_dis(gen);
		double y = random_dis(gen);
		system.addParticle(Particle(mass, x, y));
	}

	return system;
}

}  // namespace

TEST(NBodySimulatorTest, ConstructorStoresProvidedState) {
	NBodySystem system(1.0, 0.1);
	NBodySimulator simulator(&system, 2.5, 0.01);

	EXPECT_EQ(simulator.getSystem(), &system);
	EXPECT_DOUBLE_EQ(simulator.getG(), 2.5);
	EXPECT_DOUBLE_EQ(simulator.getEpsilon(), 0.01);
}

TEST(NBodySimulatorTest, MainLikeSetupSimulatesDeterministically) {
	NBodySystem system_a = CreateMainLikeSystem();
	NBodySystem system_b = CreateMainLikeSystem();

	ASSERT_EQ(system_a.getCount(), kMainParticleCount);
	ASSERT_EQ(system_b.getCount(), kMainParticleCount);

	NBodySimulator simulator_a(&system_a, kMainG, kMainEpsilon);
	NBodySimulator simulator_b(&system_b, kMainG, kMainEpsilon);

	const auto before = system_a.getBodies();

	simulator_a.simulate(0.01, 3);
	simulator_b.simulate(0.01, 3);

	ASSERT_EQ(system_a.getCount(), kMainParticleCount);
	ASSERT_EQ(system_b.getCount(), kMainParticleCount);

	bool any_body_moved = false;
	const double kTolerance = 1e-9;

	for (int i = 0; i < kMainParticleCount; ++i) {
		const Particle& a = system_a.getBodies()[i];
		const Particle& b = system_b.getBodies()[i];

		EXPECT_NEAR(a.getX(), b.getX(), kTolerance);
		EXPECT_NEAR(a.getY(), b.getY(), kTolerance);
		EXPECT_NEAR(a.getVx(), b.getVx(), kTolerance);
		EXPECT_NEAR(a.getVy(), b.getVy(), kTolerance);

		const double dx = std::fabs(a.getX() - before[i].getX());
		const double dy = std::fabs(a.getY() - before[i].getY());
		if (dx > kTolerance || dy > kTolerance) {
			any_body_moved = true;
		}
	}

	EXPECT_TRUE(any_body_moved);
}
