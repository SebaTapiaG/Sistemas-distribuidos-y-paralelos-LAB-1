#!/bin/bash
#SBATCH --job-name=nbody_cuda_job
#SBATCH --partition=GPU
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --gres=gpu:A30:1
#SBATCH --output=slurm-%A.out
#SBATCH --error=slurm-%A.err

# ==============================================================================
# SCRIPT AUXILIAR: Se escribe en /tmp y se monta en el contenedor
# Las comillas simples en 'SCRIPT_EOF' evitan que el shell externo toque nada.
# ==============================================================================
cat > /tmp/run_nbody_benchmark.sh << 'SCRIPT_EOF'
#!/bin/bash
set -euo pipefail

LOG_FILE="cluster_run.log"

# ── Cabecera de la bitácora ─────────────────────────────────────────────────
echo '=================================================================' >  "$LOG_FILE"
echo '                 BITÁCORA DE EJECUCIÓN EN CLÚSTER                ' >> "$LOG_FILE"
echo '=================================================================' >> "$LOG_FILE"
echo "Fecha y Hora de Inicio : $(date '+%Y-%m-%d %H:%M:%S')"          >> "$LOG_FILE"
echo "Usuario                : ${USER:-unknown}"                       >> "$LOG_FILE"
echo "ID del Job (SLURM)     : ${SLURM_JOB_ID:-N/A}"                 >> "$LOG_FILE"
echo "Nodo de Cómputo        : $(hostname)"                            >> "$LOG_FILE"
echo "Directorio de Trabajo  : $(pwd)"                                 >> "$LOG_FILE"
echo '-----------------------------------------------------------------' >> "$LOG_FILE"

# ── Información de GPU y entorno ─────────────────────────────────────────────
nvidia-smi                                                          >> "$LOG_FILE"
echo ''                                                             >> "$LOG_FILE"
nvcc --version                                                      >> "$LOG_FILE"
echo '=================================================================' >> "$LOG_FILE"

# ── Compilación ────────────────────────────────────────────────────────────
echo '                    COMPILACIÓN DEL PROYECTO                     ' >> "$LOG_FILE"
echo '=================================================================' >> "$LOG_FILE"

make clean >> "$LOG_FILE" 2>&1
make NVCCFLAGS='-O3 -std=c++17 -Xcompiler -Wall,-Wextra -arch=sm_80' >> "$LOG_FILE" 2>&1

echo ''                                                             >> "$LOG_FILE"
echo '=================================================================' >> "$LOG_FILE"
echo '                   EJECUCIÓN DE BENCHMARKS                       ' >> "$LOG_FILE"
echo '=================================================================' >> "$LOG_FILE"

# ── Ejecución de la suite (salida a consola Y al log) ─────────────────────
./nbody_2d_cuda -suite 2>&1 | tee -a "$LOG_FILE"

echo '=================================================================' >> "$LOG_FILE"
echo "Fecha y Hora de Término: $(date '+%Y-%m-%d %H:%M:%S')"          >> "$LOG_FILE"
echo '=================================================================' >> "$LOG_FILE"
SCRIPT_EOF

chmod +x /tmp/run_nbody_benchmark.sh

# ==============================================================================
# EJECUCIÓN EN CONTENEDOR
# ==============================================================================
srun \
  --container-image="nvidia/cuda:12.2.2-devel-ubuntu22.04" \
  --container-mounts=/tmp/run_nbody_benchmark.sh:/workspace/run_nbody_benchmark.sh,$PWD:/workspace \
  --container-workdir=/workspace \
  bash /workspace/run_nbody_benchmark.sh