CXX = g++
CXXFLAGS = -Wall -Wextra -O3 -fopenmp -std=c++17
LDFLAGS = -fopenmp
TARGET = nbody_2d
SOURCES = main.cpp Particle.cpp NBodySystem.cpp NBodySimulator.cpp Integrator.cpp MetricsCalculator.cpp Benchmark.cpp Visualizer.cpp
HEADERS = Particle.h NBodySystem.h NBodySimulator.h Integrator.h MetricsCalculator.h Benchmark.h Visualizer.h

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o *.dat *.png

benchmark: $(TARGET)
	./$(TARGET) -benchmark

analysis: $(TARGET)
	./$(TARGET) -analysis

# Enlazar con GoogleTest/Catch2 segun el proyecto; deber ́a ejecutarse en CI
test:
	# Compilar la prueba de Particle y sus dependencias.
	$(CXX) $(CXXFLAGS) -I. -o run_tests tests/TestParticle.cpp Particle.cpp $(LDFLAGS)

	./run_tests

.PHONY: clean benchmark analysis test