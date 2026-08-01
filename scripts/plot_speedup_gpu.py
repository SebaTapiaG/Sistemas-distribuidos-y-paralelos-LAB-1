import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

filename = "benchmark_results.dat"

try:
    print(f"Generando gráfico de Speedup desde {filename}...")
    # Leer archivo maestro con las desviaciones estándar
    df = pd.read_csv(filename, sep=r'\s+', comment='#', skiprows=1,
                     names=['N', 'Variant', 'BlockSize', 'CpuSerial_ms', 'CpuStdDev_ms', 
                            'GpuKernel_ms', 'KernelStdDev_ms', 'GpuE2E_ms', 'E2EStdDev_ms', 
                            'SpeedupKernel', 'SpeedupE2E', 'Amdahl_f', 'Accuracy'])
    
    df_filtered = df[df['BlockSize'] == 256].copy()
    
    # sigma_Sp = Sp * sqrt( (sigma_t1 / t1)^2 + (sigma_tp / tp)^2 )
    err_rel_cpu = np.where(df_filtered['CpuSerial_ms'] > 0, df_filtered['CpuStdDev_ms'] / df_filtered['CpuSerial_ms'], 0)
    err_rel_gpu = np.where(df_filtered['GpuE2E_ms'] > 0, df_filtered['E2EStdDev_ms'] / df_filtered['GpuE2E_ms'], 0)
    df_filtered['Sigma_Speedup'] = df_filtered['SpeedupE2E'] * np.sqrt(err_rel_cpu**2 + err_rel_gpu**2)
    
    plt.figure(figsize=(10, 6))
    
    var0 = df_filtered[df_filtered['Variant'] == 0].sort_values('N')
    var1 = df_filtered[df_filtered['Variant'] == 1].sort_values('N')
    
    # Graficar con barras de error
    plt.errorbar(var0['N'], var0['SpeedupE2E'], yerr=var0['Sigma_Speedup'], marker='o', color='royalblue', label='Kernel Básico (E2E)', capsize=4, linewidth=2)
    plt.errorbar(var1['N'], var1['SpeedupE2E'], yerr=var1['Sigma_Speedup'], marker='s', color='forestgreen', label='Shared Memory (E2E)', capsize=4, linewidth=2)
    
    plt.axhline(y=1.0, color='red', linestyle='--', label='Baseline CPU (Sp=1.0)')
    
    plt.title('Speedup GPU vs CPU frente a N con Propagación de Error', fontsize=14)
    plt.xlabel('Número de Cuerpos (N)', fontsize=12)
    plt.ylabel('Speedup', fontsize=12)
    plt.legend()
    plt.grid(True, linestyle=':', alpha=0.7)
    
    plt.savefig('plot_speedup_gpu.png', dpi=300, bbox_inches='tight')
    print("¡Éxito! Gráfico guardado como plot_speedup_gpu.png")

except Exception as e:
    print(f"Error procesando los datos: {e}")