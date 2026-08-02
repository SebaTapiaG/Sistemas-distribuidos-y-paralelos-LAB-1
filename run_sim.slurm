#!/bin/bash
#SBATCH --job-name=nbody_cuda_sim
#SBATCH --partition=GPU
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --gres=gpu:A30:1
#SBATCH --output=slurm_sim-%A.out
#SBATCH --error=slurm_sim-%A.err

LOG_FILE="cluster_sim_run.log"

srun \
  --container-image="nvidia/cuda:12.2.2-devel-ubuntu22.04" \
  --container-mounts=$PWD:/workspace \
  --container-workdir=/workspace \
  bash << 'EOF'
    LOG_FILE="cluster_sim_run.log"
    
    echo '=================================================================' > $LOG_FILE
    echo '            BITÁCORA DE EJECUCIÓN (SIMULACIÓN EN CLÚSTER)        ' >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE
    echo "Fecha y Hora de Inicio : $(date '+%Y-%m-%d %H:%M:%S')" >> $LOG_FILE
    echo "ID del Job (SLURM)     : $SLURM_JOB_ID" >> $LOG_FILE
    echo "Nodo de Cómputo        : $(hostname)" >> $LOG_FILE
    echo '-----------------------------------------------------------------' >> $LOG_FILE
    nvidia-smi >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE
    echo '                     COMPILACIÓN DEL PROYECTO                    ' >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE

    # Instalación rápida de dependencias dentro del contenedor
    apt-get update && apt-get install -y --no-install-recommends g++ make libomp-dev >> $LOG_FILE 2>&1

    # Compilación ajustada para la GPU A30 (sm_80)
    make clean >> $LOG_FILE 2>&1
    make NVCCFLAGS='-O3 -std=c++17 -Xcompiler -Wall,-Wextra -arch=sm_80' >> $LOG_FILE 2>&1

    echo '=================================================================' >> $LOG_FILE
    echo '                    EJECUCIÓN DE SIMULACIÓN (-sim)              ' >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE

    # Ejecución del modo simulación registrando salida e historial
    ./nbody_2d_cuda -sim 2>&1 | tee -a $LOG_FILE

    echo '=================================================================' >> $LOG_FILE
    echo "Fecha y Hora de Término: $(date '+%Y-%m-%d %H:%M:%S')" >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE
EOF#!/bin/bash
#SBATCH --job-name=nbody_cuda_sim
#SBATCH --partition=GPU
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --gres=gpu:A30:1
#SBATCH --output=slurm_sim-%A.out
#SBATCH --error=slurm_sim-%A.err

LOG_FILE="cluster_sim_run.log"

srun \
  --container-image="nvidia/cuda:12.2.2-devel-ubuntu22.04" \
  --container-mounts=$PWD:/workspace \
  --container-workdir=/workspace \
  bash << 'EOF'
    LOG_FILE="cluster_sim_run.log"
    
    echo '=================================================================' > $LOG_FILE
    echo '            BITÁCORA DE EJECUCIÓN (SIMULACIÓN EN CLÚSTER)        ' >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE
    echo "Fecha y Hora de Inicio : $(date '+%Y-%m-%d %H:%M:%S')" >> $LOG_FILE
    echo "ID del Job (SLURM)     : $SLURM_JOB_ID" >> $LOG_FILE
    echo "Nodo de Cómputo        : $(hostname)" >> $LOG_FILE
    echo '-----------------------------------------------------------------' >> $LOG_FILE
    nvidia-smi >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE
    echo '                     COMPILACIÓN DEL PROYECTO                    ' >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE

    # Instalación rápida de dependencias dentro del contenedor
    apt-get update && apt-get install -y --no-install-recommends g++ make libomp-dev >> $LOG_FILE 2>&1

    # Compilación ajustada para la GPU A30 (sm_80)
    make clean >> $LOG_FILE 2>&1
    make NVCCFLAGS='-O3 -std=c++17 -Xcompiler -Wall,-Wextra -arch=sm_80' >> $LOG_FILE 2>&1

    echo '=================================================================' >> $LOG_FILE
    echo '                    EJECUCIÓN DE SIMULACIÓN (-sim)              ' >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE

    # Ejecución del modo simulación registrando salida e historial
    ./nbody_2d_cuda -sim 2>&1 | tee -a $LOG_FILE

    echo '=================================================================' >> $LOG_FILE
    echo "Fecha y Hora de Término: $(date '+%Y-%m-%d %H:%M:%S')" >> $LOG_FILE
    echo '=================================================================' >> $LOG_FILE
EOF