#!/bin/bash
#SBATCH --job-name=nbody_atomic_validation
#SBATCH --partition=GPU
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --gres=gpu:A30:1
#SBATCH --output=slurm-atomic-%A.out
#SBATCH --error=slurm-atomic-%A.err

# Rescatamos el usuario del clúster antes de invocar el contenedor
REAL_USER=${SLURM_JOB_USER:-$USER}

srun \
  --container-image="nvidia/cuda:12.2.2-devel-ubuntu22.04" \
  --container-mounts=$PWD:/workspace \
  --container-workdir=/workspace \
  bash << EOF
    set -euo pipefail

    LOG_FILE="atomic_validation.log"
    N=1024
    VARIANT=0
    BLOCK=256
    RUNS=5

    # ── Cabecera de la bitácora ─────────────────────────────────────────────────
    echo '=================================================================' >  "\$LOG_FILE"
    echo '        VALIDACIÓN ATOMICADD vs REDUCCIÓN EN SHARED MEMORY       ' >> "\$LOG_FILE"
    echo '=================================================================' >> "\$LOG_FILE"
    echo "Fecha y Hora de Inicio : \$(date '+%Y-%m-%d %H:%M:%S')"          >> "\$LOG_FILE"
    echo "Usuario                : $REAL_USER"                             >> "\$LOG_FILE"
    echo "ID del Job (SLURM)     : \$SLURM_JOB_ID"                         >> "\$LOG_FILE"
    echo "Nodo                   : \$(hostname)"                            >> "\$LOG_FILE"
    echo "GPU                    : \$(nvidia-smi --query-gpu=name --format=csv,noheader | head -n1)" >> "\$LOG_FILE"
    echo "Config                 : N=\$N, Variant=\$VARIANT, Block=\$BLOCK, Runs=\$RUNS" >> "\$LOG_FILE"
    echo '-----------------------------------------------------------------' >> "\$LOG_FILE"

    # ── Instalación de dependencias y Compilación ──────────────────────────────
    apt-get update && apt-get install -y --no-install-recommends g++ make libomp-dev >> "\$LOG_FILE" 2>&1

    make clean >> "\$LOG_FILE" 2>&1
    make NVCCFLAGS='-O3 -std=c++17 -Xcompiler -Wall,-Wextra -arch=sm_80' >> "\$LOG_FILE" 2>&1

    # ── Ejecución de Pruebas ──────────────────────────────────────────────────
    echo '' >> "\$LOG_FILE"
    echo '--- 1. BASELINE: energy_method=0 (Reducción en Shared Memory) ---' >> "\$LOG_FILE"
    ./nbody_2d_cuda -test \$N \$VARIANT \$BLOCK \$RUNS 0 2>&1 | tee -a "\$LOG_FILE"

    echo '' >> "\$LOG_FILE"
    echo '--- 2. PRUEBA: energy_method=1 (atomicAdd) ---------------------' >> "\$LOG_FILE"
    ./nbody_2d_cuda -test \$N \$VARIANT \$BLOCK \$RUNS 1 2>&1 | tee -a "\$LOG_FILE"

    echo '' >> "\$LOG_FILE"
    echo '=================================================================' >> "\$LOG_FILE"
    echo "Fecha y Hora de Término: \$(date '+%Y-%m-%d %H:%M:%S')"          >> "\$LOG_FILE"
    echo '=================================================================' >> "\$LOG_FILE"
EOF