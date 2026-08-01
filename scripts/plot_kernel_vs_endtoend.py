import pandas as pd
import matplotlib.pyplot as plt

filename = "benchmark_results.dat"

try:
    print(f"Generando impacto de transferencias desde {filename}...")
    df = pd.read_csv(filename, sep=r'\s+', comment='#', skiprows=1,
                     names=['N', 'Variant', 'BlockSize', 'CpuSerial_ms', 'CpuStdDev_ms', 
                            'GpuKernel_ms', 'KernelStdDev_ms', 'GpuE2E_ms', 'E2EStdDev_ms', 
                            'SpeedupKernel', 'SpeedupE2E', 'Amdahl_f', 'Accuracy'])
    
    df_var = df[(df['Variant'] == 1) & (df['BlockSize'] == 256)].sort_values('N')
    
    plt.figure(figsize=(10, 6))
    
    # Graficar con barras de error
    plt.errorbar(df_var['N'], df_var['GpuE2E_ms'], yerr=df_var['E2EStdDev_ms'], marker='o', color='purple', label='End-to-End (Incluye cudaMemcpy)', capsize=4, linewidth=2)
    plt.errorbar(df_var['N'], df_var['GpuKernel_ms'], yerr=df_var['KernelStdDev_ms'], marker='^', color='orange', label='Kernel-Only (Puro Cómputo)', capsize=4, linewidth=2)
    
    plt.fill_between(df_var['N'], df_var['GpuKernel_ms'], df_var['GpuE2E_ms'], color='gray', alpha=0.2, label='Overhead de Transferencias')
    
    plt.title('Impacto de Transferencias Host-Device (Shared Memory)', fontsize=14)
    plt.xlabel('Número de Cuerpos (N)', fontsize=12)
    plt.ylabel('Tiempo Medio (ms)', fontsize=12)
    plt.legend()
    plt.grid(True, linestyle=':', alpha=0.7)
    
    plt.savefig('plot_kernel_vs_endtoend.png', dpi=300, bbox_inches='tight')
    print("¡Éxito! Gráfico guardado como plot_kernel_vs_endtoend.png")

except Exception as e:
    print(f"Error procesando los datos: {e}")