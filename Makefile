CXX = g++
CXXFLAGS = -Wall -Wextra -O3 -fopenmp -std=c++17
LDFLAGS = -fopenmp
TARGET = nbody_2d
SOURCES = Altmain.cpp Particle.cpp NBodySystem.cpp NBodySimulator.cpp MetricsCalculator.cpp Benchmark.cpp Visualizer.cpp
HEADERS = Particle.h NBodySystem.h NBodySimulator.h MetricsCalculator.h Benchmark.h Visualizer.h

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o *.dat *.png

benchmark: $(TARGET)
	./$(TARGET) -benchmark

analysis: $(TARGET)
	./$(TARGET) -analysis

# Enlazar con GoogleTest/Catch2 segun el proyecto; debera ejecutarse en CI
test:
	# Compilar las pruebas de Particle, NBodySystem y NBodySimulator.
	$(CXX) $(CXXFLAGS) -I. -o run_tests tests/TestParticle.cpp tests/TestNBodySystem.cpp tests/TestNBodySimulator.cpp tests/TestVisualizer.cpp tests/TestMetricsCalculator.cpp tests/TestBenchmark.cpp Particle.cpp NBodySystem.cpp NBodySimulator.cpp Visualizer.cpp MetricsCalculator.cpp Benchmark.cpp $(LDFLAGS) -lgtest -lgtest_main -pthread

	./run_tests

.PHONY: clean benchmark analysis run_tests