import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

filename = "blockdim_study.dat"

try:
    print(f"Generando estudio de blockDim desde {filename}...")
    df = pd.read_csv(filename, sep=r'\s+', comment='#', skiprows=1,
                     names=['BlockSize', 'Variant', 'N', 'GpuKernel_ms', 'KernelStdDev', 
                            'GpuE2E_ms', 'E2EStdDev', 'SpeedupKernel', 'SpeedupE2E', 'Amdahl_f', 'Accuracy'])
    
    plt.figure(figsize=(10, 6))
    
    var0 = df[df['Variant'] == 0].sort_values('BlockSize')
    var1 = df[df['Variant'] == 1].sort_values('BlockSize')
    
    bar_width = 0.35
    index = np.arange(len(var0['BlockSize']))
    
    plt.bar(index, var0['GpuKernel_ms'], bar_width, color='royalblue', label='Kernel Básico', yerr=var0['KernelStdDev'], capsize=5)
    plt.bar(index + bar_width, var1['GpuKernel_ms'], bar_width, color='forestgreen', label='Shared Memory', yerr=var1['KernelStdDev'], capsize=5)
    
    plt.title(f"Tiempo de Ejecución de Kernels vs blockDim.x (N = {var0['N'].iloc[0]})", fontsize=14)
    plt.xlabel('blockDim.x (Hilos por bloque)', fontsize=12)
    plt.ylabel('Tiempo Kernel-Only (ms)', fontsize=12)
    plt.xticks(index + bar_width / 2, var0['BlockSize'])
    plt.legend()
    plt.grid(axis='y', linestyle=':', alpha=0.7)
    
    plt.savefig('plot_blockdim.png', dpi=300, bbox_inches='tight')
    print("¡Éxito! Gráfico guardado como plot_blockdim.png")

except Exception as e:
    print(f"Error procesando los datos: {e}")