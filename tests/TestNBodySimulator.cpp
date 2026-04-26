#include <gtest/gtest.h>
#include <cmath>
#include <random>

#include "NBodySimulator.h"

// pronto

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
}