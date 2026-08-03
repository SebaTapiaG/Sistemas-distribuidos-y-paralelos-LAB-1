#!/bin/bash
#SBATCH --job-name=nbody_atomic_validation
#SBATCH --partition=GPU
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --gres=gpu:A30:1
#SBATCH --output=slurm-atomic-%A.out
#SBATCH --error=slurm-atomic-%A.err

cat > /tmp/run_atomic_test.sh << 'SCRIPT_EOF'
#!/bin/bash
set -euo pipefail

LOG_FILE="atomic_validation.log"
N=1024
VARIANT=0
BLOCK=256
RUNS=10

echo '=================================================================' >  "$LOG_FILE"
echo '        VALIDACIÓN ATOMICADD vs REDUCCIÓN EN SHARED MEMORY       ' >> "$LOG_FILE"
echo '=================================================================' >> "$LOG_FILE"
echo "Fecha y Hora de Inicio : $(date '+%Y-%m-%d %H:%M:%S')"          >> "$LOG_FILE"
echo "Nodo                   : $(hostname)"                             >> "$LOG_FILE"
echo "GPU                    : $(nvidia-smi --query-gpu=name --format=csv,noheader | head -n1)" >> "$LOG_FILE"
echo "Config                 : N=$N, Variant=$VARIANT, Block=$BLOCK, Runs=$RUNS" >> "$LOG_FILE"
echo '-----------------------------------------------------------------' >> "$LOG_FILE"

# Compilación
make clean
make NVCCFLAGS='-O3 -std=c++17 -Xcompiler -Wall,-Wextra -arch=sm_80' >> "$LOG_FILE" 2>&1

echo '' >> "$LOG_FILE"
echo '--- 1. BASELINE: energy_method=0 (Reducción en Shared Memory) ---' >> "$LOG_FILE"
./nbody_2d_cuda -test $N $VARIANT $BLOCK $RUNS 0 | tee -a "$LOG_FILE"

echo '' >> "$LOG_FILE"
echo '--- 2. PRUEBA: energy_method=1 (atomicAdd) ---------------------' >> "$LOG_FILE"
./nbody_2d_cuda -test $N $VARIANT $BLOCK $RUNS 1 | tee -a "$LOG_FILE"

echo '' >> "$LOG_FILE"
echo '=================================================================' >> "$LOG_FILE"
echo "Fecha y Hora de Término: $(date '+%Y-%m-%d %H:%M:%S')"          >> "$LOG_FILE"
echo '=================================================================' >> "$LOG_FILE"
SCRIPT_EOF

chmod +x /tmp/run_atomic_test.sh

srun \
  --container-image="nvidia/cuda:12.2.2-devel-ubuntu22.04" \
  --container-mounts=/tmp/run_atomic_test.sh:/workspace/run_atomic_test.sh,$PWD:/workspace \
  --container-workdir=/workspace \
  bash /workspace/run_atomic_test.sh