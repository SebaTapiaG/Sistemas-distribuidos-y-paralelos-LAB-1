import pandas as pd
import matplotlib.pyplot as plt

filename = "benchmark_results.dat"

try:
    print(f"Generando comparación Básica vs Shared desde {filename}...")
    df = pd.read_csv(filename, sep=r'\s+', comment='#', skiprows=1,
                     names=['N', 'Variant', 'BlockSize', 'CpuSerial_ms', 'CpuStdDev_ms', 
                            'GpuKernel_ms', 'KernelStdDev_ms', 'GpuE2E_ms', 'E2EStdDev_ms', 
                            'SpeedupKernel', 'SpeedupE2E', 'Amdahl_f', 'Accuracy'])
    
    df_filtered = df[df['BlockSize'] == 256]
    var0 = df_filtered[df_filtered['Variant'] == 0].sort_values('N')
    var1 = df_filtered[df_filtered['Variant'] == 1].sort_values('N')
    
    plt.figure(figsize=(10, 6))
    
    plt.errorbar(var0['N'], var0['GpuKernel_ms'], yerr=var0['KernelStdDev_ms'], marker='o', color='royalblue', label='Variante Básica (Global Memory)', capsize=4, linewidth=2)
    plt.errorbar(var1['N'], var1['GpuKernel_ms'], yerr=var1['KernelStdDev_ms'], marker='s', color='forestgreen', label='Variante Optimizada (Shared Memory)', capsize=4, linewidth=2)
    
    plt.title('Comparación de Tiempo de Cómputo: Básico vs. Shared Memory', fontsize=14)
    plt.xlabel('Número de Cuerpos (N)', fontsize=12)
    plt.ylabel('Tiempo Kernel-Only (ms)', fontsize=12)
    plt.legend()
    plt.grid(True, linestyle=':', alpha=0.7)
    
    plt.savefig('plot_basic_vs_shared.png', dpi=300, bbox_inches='tight')
    print("¡Éxito! Gráfico guardado como plot_basic_vs_shared.png")

except Exception as e:
    print(f"Error procesando los datos: {e}")