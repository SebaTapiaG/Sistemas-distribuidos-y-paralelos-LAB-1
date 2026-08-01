#!/bin/bash
#SBATCH --job-name=nbody_cuda
#SBATCH --partition=GPU
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8          # 8 hilos OpenMP
#SBATCH --gres=gpu:A30:1            # 1 GPU NVIDIA A30
#SBATCH --time=01:30:00             # Tiempo máximo:1 hora 30 minutos
#SBATCH --output=logs/cluster_run_%j.log
#SBATCH --error=logs/cluster_run_%j.err

# Configuración de hilos OpenMP
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

# ==============================================================================
# ENCABEZADO DEL LOG (Información de auditoría)
# ==============================================================================
echo "======================================================================"
echo "                   REGISTRO DE EJECUCIÓN EN CLUSTER                  "
echo "======================================================================"
echo "FECHA / HORA INICIO : $(date '+%Y-%m-%d %H:%M:%S')"
echo "JOB ID              : $SLURM_JOB_ID"
echo "NODO ASIGNADO       : $SLURMD_NODENAME ($(hostname))"
echo "PARTICIÓN           : $SLURM_JOB_PARTITION"
echo "CPUS ASIGNADAS      : $SLURM_CPUS_PER_TASK hilos OpenMP"
echo "DIRECTORIO DE TRABAJO: $SLURM_SUBMIT_DIR"
echo "----------------------------------------------------------------------"
echo "INFORMACIÓN DE LA GPU ASIGNADA:"
nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv,noheader
echo "======================================================================"
echo ""

# ==============================================================================
# COMPILACIÓN Y EJECUCIÓN
# ==============================================================================

echo ">>> [1/3] Limpiando y Compilando el código..."
# Compilar especificando la arquitectura de la NVIDIA A30 (sm_80)
make clean
make NVCCFLAGS="-O3 -std=c++17 -Xcompiler -Wall,-Wextra -arch=sm_80"

echo -e "\n>>> [2/3] Ejecutando Tests de Verificación..."
make test

echo -e "\n>>> [3/3] Ejecutando Benchmark Suite..."
time ./nbody_2d_cuda -suite

echo ""
echo "======================================================================"
echo "FECHA / HORA FIN    : $(date '+%Y-%m-%d %H:%M:%S')"
echo "ESTADO FINAL        : Ejecución finalizada con código de salida $?"
echo "======================================================================"
