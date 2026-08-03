# ==============================================================================
# COMPILADORES Y BANDERAS
# ==============================================================================
NVCC        = nvcc
CXX         = g++

CUDA_PATH   ?= /usr/local/cuda

# Banderas para CUDA (NVCC) y C++ con OpenMP (G++)
# Note: -I$(CUDA_PATH)/include permite a g++ encontrar <cuda_runtime.h>
NVCCFLAGS    = -O3 -std=c++17 -Xcompiler -Wall,-Wextra  -arch=sm_80
CXXFLAGS     = -Wall -Wextra -O3 -fopenmp -std=c++17 -I$(CUDA_PATH)/include

LDFLAGS      = -fopenmp -L$(CUDA_PATH)/lib64 -lcudart
CUDA_LDFLAGS = -L$(CUDA_PATH)/lib64 -lcudart

# ==============================================================================
# ARCHIVOS Y OBJETIVOS
# ==============================================================================
TARGET      = nbody_2d_cuda

CPP_SOURCES = main.cpp Particle.cpp NBodySystem.cpp NBodySimulator.cpp \
              MetricsCalculator.cpp Benchmark.cpp Visualizer.cpp

CU_SOURCES  = kernels/accelerations.cu kernels/integration.cu kernels/energy.cu

HEADERS     = Particle.h NBodySystem.h NBodySimulator.h MetricsCalculator.h \
              Benchmark.h Visualizer.h CudaBuffer.h \
              kernels/accelerations.cuh kernels/integration.cuh kernels/energy.cuh

# Arreglos de objetos (.o)
CPP_OBJS    = $(CPP_SOURCES:.cpp=.o)
CU_OBJS     = $(CU_SOURCES:.cu=.o)

# ==============================================================================
# REGLAS DE COMPILACIÓN PRINCIPALES
# ==============================================================================

# Regla principal: enlaza objetos C++ (OpenMP) y CUDA
$(TARGET): $(CPP_OBJS) $(CU_OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(CPP_OBJS) $(CU_OBJS) $(LDFLAGS)

# Compilación de módulos C++ (.cpp -> .o)
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compilación de kernels CUDA (.cu -> .o)
kernels/%.o: kernels/%.cu $(HEADERS)
	$(NVCC) $(NVCCFLAGS) -c $< -o $@

# ==============================================================================
# OBJETIVOS AUXILIARES Y PRUEBAS AUTOMÁTICAS
# ==============================================================================

clean:
	rm -f $(TARGET) run_tests run_cuda_test run_integration_test run_energy_test test_kernel_basico test_kernel_basico.exe *.o kernels/*.o *.dat *.png

benchmark: $(TARGET)
	./$(TARGET) -benchmark

analysis: $(TARGET)
	./$(TARGET) -analysis
# Corre la suite completa de benchmarks en GPU (-suite)
suite: $(TARGET)
	./$(TARGET) -suite

# Mantiene compatibilidad al llamar 'make benchmark'
benchmark: $(TARGET)
	./$(TARGET) -suite

# Corre una prueba única puntual (-test N variant blocksize runs)
test_single: $(TARGET)
	./$(TARGET) -test 1024 0 256 10

#-sim para visualizer

# Regla rápida para compilar y ejecutar SOLO el test de Aceleraciones
test_basico: $(CU_OBJS)
	$(NVCC) $(NVCCFLAGS) -o run_cuda_test tests/test_kernel_basico.cu kernels/accelerations.cu $(CUDA_LDFLAGS)
	./run_cuda_test

# Regla rápida para compilar y ejecutar SOLO el test de Integración (Euler)
test_integration: $(CU_OBJS)
	$(NVCC) $(NVCCFLAGS) -o run_integration_test tests/test_integrator.cu kernels/integration.cu $(CUDA_LDFLAGS)
	./run_integration_test

# Regla rápida para compilar y ejecutar SOLO el test de Energía
test_energy: $(CU_OBJS)
	$(NVCC) $(NVCCFLAGS) -o run_energy_test tests/test_kernel_energy.cu kernels/energy.cu $(CUDA_LDFLAGS)
	./run_energy_test

# ── Regla de Test GPU (Compilada y Enlazada Correctamente) ───────────────────
test_simulation: Particle.o NBodySystem.o NBodySimulator.o
	$(NVCC) $(NVCCFLAGS) -Xcompiler -fopenmp -o run_sim_test tests/test_gpu_simulation.cu \
		Particle.o NBodySystem.o NBodySimulator.o \
		kernels/accelerations.cu kernels/integration.cu kernels/energy.cu \
		-L/usr/local/cuda/lib64 -lcudart -lgomp
	./run_sim_test

# Ejecuta tanto la suite GoogleTest como todos los tests unitarios de CUDA
test: $(CU_OBJS)
	# 0.1 Compilar kernels CUDA a archivos objeto
	$(NVCC) $(NVCCFLAGS) -c kernels/accelerations.cu -o accelerations.o
	$(NVCC) $(NVCCFLAGS) -c kernels/integration.cu -o integration.o
	$(NVCC) $(NVCCFLAGS) -c kernels/energy.cu -o energy.o

	# 1. Pruebas unitarias C++/OpenMP/CUDA con GoogleTest
	$(CXX) $(CXXFLAGS) -I. -o run_tests \
		tests/TestParticle.cpp tests/TestNBodySystem.cpp tests/TestNBodySimulator.cpp \
		tests/TestVisualizer.cpp tests/TestMetricsCalculator.cpp tests/TestBenchmark.cpp \
		tests/TestBenchmarkGPU.cpp \
		tests/TestSuiteGPU.cpp \
		Particle.cpp NBodySystem.cpp NBodySimulator.cpp Visualizer.cpp MetricsCalculator.cpp Benchmark.cpp \
		kernels/accelerations.o kernels/integration.o kernels/energy.o \
		-L/usr/local/cuda/lib64 -lcudart $(LDFLAGS) -lgtest -lgtest_main -pthread
	./run_tests

	# 2. Test unitario de Aceleraciones
	@if [ -f tests/test_kernel_basico.cu ]; then \
		echo "\n--- Ejecutando pruebas de test_kernel_basico.cu ---"; \
		$(NVCC) $(NVCCFLAGS) -o run_cuda_test tests/test_kernel_basico.cu kernels/accelerations.cu $(CUDA_LDFLAGS); \
		./run_cuda_test; \
	fi

	# 3. Test unitario de Integración
	@if [ -f tests/test_integrator.cu ]; then \
		echo "\n--- Ejecutando pruebas de test_kernel_integrator.cu ---"; \
		$(NVCC) $(NVCCFLAGS) -o run_integration_test tests/test_integrator.cu kernels/integration.cu $(CUDA_LDFLAGS); \
		./run_integration_test; \
	fi

	# 4. Test unitario de Energía
	@if [ -f tests/test_kernel_energy.cu ]; then \
		echo "\n--- Ejecutando pruebas de test_kernel_energy.cu ---"; \
		$(NVCC) $(NVCCFLAGS) -o run_energy_test tests/test_kernel_energy.cu kernels/energy.cu $(CUDA_LDFLAGS); \
		./run_energy_test; \
	fi

.PHONY: clean benchmark suite test_single sim analysis test test_basico test_integration test_energy test_simulation