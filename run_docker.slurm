#!/bin/bash
#SBATCH --job-name=nbody_cuda_job
#SBATCH --partition=GPU
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --gres=gpu:A30:1
#SBATCH --output=slurm-%A.out
#SBATCH --error=slurm-%A.err

LOG_FILE="cluster_run.log"

srun \
  --container-image="nvidia/cuda:12.2.2-devel-ubuntu22.04" \
  --container-mounts=$PWD:/workspace \
  --container-workdir=/workspace \
  bash -c "
    echo '=================================================================' > $LOG_FILE
    echo '                 BITÁCORA DE EJECUCIÓN EN CLÚSTER                ' >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE
    echo 'Fecha y Hora de Inicio : \$(date \"+%Y-%m-%d %H:%M:%S\")' >> $LOG_FILE
    echo 'Usuario                : \$USER' >> $LOG_FILE
    echo 'ID del Job (SLURM)     : \$SLURM_JOB_ID' >> $LOG_FILE
    echo 'Nodo de Cómputo        : \$(hostname)' >> $LOG_FILE
    echo '-----------------------------------------------------------------' >> $LOG_FILE
    nvidia-smi >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE
    echo '                    COMPILACIÓN DEL PROYECTO                     ' >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE

    # Instalación rápida de dependencias dentro del contenedor en tiempo de ejecución
    apt-get update && apt-get install -y --no-install-recommends g++ make libomp-dev >> $LOG_FILE 2>&1

    # Compilación ajustada para la GPU A30 (sm_80)
    make clean >> $LOG_FILE 2>&1
    make NVCCFLAGS='-O3 -std=c++17 -Xcompiler -Wall,-Wextra -arch=sm_80' >> $LOG_FILE 2>&1

    echo '=================================================================' >> $LOG_FILE
    echo '                   EJECUCIÓN DE BENCHMARKS                       ' >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE

    # Ejecución de la suite completa registrando en cluster_run.log y pantalla
    ./nbody_2d_cuda -suite 2>&1 | tee -a $LOG_FILE

    echo '=================================================================' >> $LOG_FILE
    echo 'Fecha y Hora de Término: \$(date \"+%Y-%m-%d %H:%M:%S\")' >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE
  "
