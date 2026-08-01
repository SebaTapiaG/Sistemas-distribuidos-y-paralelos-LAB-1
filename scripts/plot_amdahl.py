import pandas as pd
import matplotlib.pyplot as plt

filename = "scaling_analysis.dat"

try:
    print(f"Generando Curva de Amdahl desde {filename}...")
    # Leer las 12 columnas definidas en Benchmark.cpp
    df = pd.read_csv(filename, sep=r'\s+', comment='#', skiprows=1,
                     names=['N', 'Variant', 'BlockSize', 'CpuTime_ms', 'GpuKernel_ms', 
                            'GpuE2E_ms', 'Overhead_ms', 'Serial_frac', 'SpeedupKernel', 
                            'TheoLimit_K', 'SpeedupE2E', 'TheoLimit_E2E'])
    
    # Filtramos para analizar la variante de Shared Memory (Variant=1) con un bloque estándar (256)
    df_var = df[(df['Variant'] == 1) & (df['BlockSize'] == 256)].sort_values('N')
    
    plt.figure(figsize=(10, 6))
    
    # 1. Curva de Medición Real
    plt.plot(df_var['N'], df_var['SpeedupE2E'], marker='o', color='blue', label='Medición Real (Speedup End-to-End)', linewidth=2)
    
    # 2. Curva de Predicción Teórica (Ley de Amdahl)
    plt.plot(df_var['N'], df_var['TheoLimit_E2E'], marker='s', color='red', linestyle='--', label='Predicción Teórica (Amdahl)', linewidth=2)
    
    # Formato profesional
    plt.title('Ley de Amdahl: Predicción vs Medición Real (Shared Memory)', fontsize=14)
    plt.xlabel('Número de Cuerpos (N)', fontsize=12)
    plt.ylabel('Speedup', fontsize=12)
    plt.legend()
    plt.grid(True, linestyle=':', alpha=0.7)
    
    output_file = 'plot_amdahl.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"¡Éxito! Gráfico guardado como {output_file}")

except Exception as e:
    print(f"Error procesando los datos: {e}")