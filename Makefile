# ==============================================================================
# COMPILADORES Y BANDERAS
# ==============================================================================
NVCC        = nvcc
CXX         = g++

# Banderas para CUDA (NVCC) y C++ con OpenMP (G++)
NVCCFLAGS    = -O3 -std=c++17 -Xcompiler -Wall,-Wextra
CXXFLAGS     = -Wall -Wextra -O3 -fopenmp -std=c++17
#LDFLAGS      = -fopenmp -lcudart
#CUDA_LDFLAGS = -lcudart
CUDA_PATH    ?= /usr/local/cuda

LDFLAGS      = -fopenmp -L$(CUDA_PATH)/lib64 -lcudart
CUDA_LDFLAGS = -L$(CUDA_PATH)/lib64 -lcudart

# ==============================================================================
# ARCHIVOS Y OBJETIVOS
# ==============================================================================
TARGET      = nbody_2d_cuda

CPP_SOURCES = Altmain.cpp Particle.cpp NBodySystem.cpp NBodySimulator.cpp \
              MetricsCalculator.cpp Benchmark.cpp Visualizer.cpp

CU_SOURCES  = kernels/accelerations.cu

HEADERS     = Particle.h NBodySystem.h NBodySimulator.h MetricsCalculator.h \
              Benchmark.h Visualizer.h kernels/accelerations.cuh

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
	rm -f $(TARGET) run_tests run_cuda_test test_kernel_basico test_kernel_basico.exe *.o kernels/*.o *.dat *.png

benchmark: $(TARGET)
	./$(TARGET) -benchmark

analysis: $(TARGET)
	./$(TARGET) -analysis

# Regla rápida para compilar y ejecutar SOLO el test del Kernel Básico CUDA
test_basico: $(CU_OBJS)
	$(NVCC) $(NVCCFLAGS) -o run_cuda_test tests/test_kernel_basico.cu kernels/accelerations.cu $(CUDA_LDFLAGS)
	./run_cuda_test

# Ejecuta tanto la suite GoogleTest (Lab 1) como el test de Kernel Básico de CUDA (Rol 1)
test: $(CU_OBJS)
	# 1. Pruebas unitarias originales C++/OpenMP con GoogleTest
	$(CXX) $(CXXFLAGS) -I. -o run_tests \
		tests/TestParticle.cpp tests/TestNBodySystem.cpp tests/TestNBodySimulator.cpp \
		tests/TestVisualizer.cpp tests/TestMetricsCalculator.cpp tests/TestBenchmark.cpp \
		Particle.cpp NBodySystem.cpp NBodySimulator.cpp Visualizer.cpp MetricsCalculator.cpp Benchmark.cpp \
		$(LDFLAGS) -lgtest -lgtest_main -pthread
	./run_tests

	# 2. Test unitario de verificación del Kernel Básico de CUDA (Rol 1)
	@if [ -f tests/test_kernel_basico.cu ]; then \
		echo "\n--- Ejecutando pruebas de test_kernel_basico.cu ---"; \
		$(NVCC) $(NVCCFLAGS) -o run_cuda_test tests/test_kernel_basico.cu kernels/accelerations.cu $(CUDA_LDFLAGS); \
		./run_cuda_test; \
	fi

.PHONY: clean benchmark analysis test test_basico