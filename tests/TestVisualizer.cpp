#include <gtest/gtest.h>

#include <array>
#include <cstdio>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "NBodySimulator.h"
#include "Visualizer.h"

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
		const double mass = random_mass(gen);
		const double x = random_dis(gen);
		const double y = random_dis(gen);
		system.addParticle(Particle(mass, x, y));
	}

	return system;
}

std::vector<std::array<double, 8>> ReadSnapshot(const std::string& path) {
	std::ifstream input(path);
	std::vector<std::array<double, 8>> rows;

	std::string line;
	while (std::getline(input, line)) {
		if (line.empty()) {
			continue;
		}

		std::istringstream iss(line);
		std::array<double, 8> row{};
		bool ok = true;
		for (double& value : row) {
			if (!(iss >> value)) {
				ok = false;
				break;
			}
		}
		if (ok) {
			rows.push_back(row);
		}
	}

	return rows;
}

TEST(VisualizerTest, CapturaEstadoDeSimulacionMainLike) {
    const char* kSnapshotFile = "Snapshot.dat";
    std::remove(kSnapshotFile);
    NBodySystem system = CreateMainLikeSystem();
    
    
    NBodySimulator simulator(&system, 0.01); 
    
    
    simulator.processBodies(3); 
    
    Visualizer visualizer;
    visualizer.abrirArchivo(); 
    visualizer.capturarEstado(system);
    visualizer.cerrarArchivo();
    
    std::ifstream snapshot(kSnapshotFile);
    ASSERT_TRUE(snapshot.is_open());
    snapshot.close();
    
    const auto rows = ReadSnapshot(kSnapshotFile);
    ASSERT_EQ(rows.size(), static_cast<size_t>(kMainParticleCount));
    
    const auto& bodies = system.getBodies();
    const double kTolerance = 1e-4;
    
    EXPECT_NEAR(rows.front()[0], 0.0, kTolerance);
    EXPECT_NEAR(rows.front()[1], bodies.front().getMass(), kTolerance);
    EXPECT_NEAR(rows.front()[2], bodies.front().getX(), kTolerance);
    EXPECT_NEAR(rows.front()[3], bodies.front().getY(), kTolerance);
    EXPECT_NEAR(rows.front()[4], bodies.front().getVx(), kTolerance);
    EXPECT_NEAR(rows.front()[5], bodies.front().getVy(), kTolerance);
    EXPECT_NEAR(rows.front()[6], bodies.front().getAx(), kTolerance);
    EXPECT_NEAR(rows.front()[7], bodies.front().getAy(), kTolerance);
    
    EXPECT_NEAR(rows.back()[0], static_cast<double>(kMainParticleCount - 1), kTolerance);
    EXPECT_NEAR(rows.back()[1], bodies.back().getMass(), kTolerance);
    EXPECT_NEAR(rows.back()[2], bodies.back().getX(), kTolerance);
    EXPECT_NEAR(rows.back()[3], bodies.back().getY(), kTolerance);
    EXPECT_NEAR(rows.back()[4], bodies.back().getVx(), kTolerance);
    EXPECT_NEAR(rows.back()[5], bodies.back().getVy(), kTolerance);
    EXPECT_NEAR(rows.back()[6], bodies.back().getAx(), kTolerance);
    EXPECT_NEAR(rows.back()[7], bodies.back().getAy(), kTolerance);
    
    std::remove(kSnapshotFile);
}

TEST(VisualizerTest, ExportarTrayectoriasEscribeFormatoCorrecto) {
    Visualizer vis;
    simulation_data data;
    std::string test_filename = "test_trayectorias.dat";

    // 1. Setup: Crear un estado de simulación falso (mock)
    std::vector<Particle> particulas_paso_cero;
    Particle p1(5.0, 10.0, -10.0); 
    particulas_paso_cero.push_back(p1);
    
    data.bodies.push_back(particulas_paso_cero);

    // 2. Ejecución: Llamar a la nueva función
    vis.exportarTrayectorias(data, test_filename);

    // 3. Verificación (Asserts)
    std::ifstream f(test_filename);
    ASSERT_TRUE(f.is_open()) << "Error: El archivo de trayectorias no se genero.";

    std::string header;
    std::getline(f, header);
    // Verificar que el encabezado tiene la nueva columna ID para que el subsampling en Python funcione
    EXPECT_EQ(header, "# Paso ID Masa X Y Vx Vy");

    // Verificar los datos de la partícula
    int step, id;
    double mass, x, y, vx, vy;
    f >> step >> id >> mass >> x >> y >> vx >> vy;

    EXPECT_EQ(step, 0);
    EXPECT_EQ(id, 0);
    EXPECT_DOUBLE_EQ(mass, p1.getMass());
    EXPECT_DOUBLE_EQ(x, p1.getX());
    EXPECT_DOUBLE_EQ(y, p1.getY());
    
    f.close();

    // 4. Limpieza
    std::remove(test_filename.c_str());
}

}  // namespace

