import matplotlib.pyplot as plt
import pandas as pd
import numpy as np  

filename_scal = "scaling_analysis.dat"
filename_bench = "bench_results.dat"

try:
    print("Generando análisis de balanceo de carga (Tiempo vs Chunk por Schedule)...")
    
    # 1. Cargar ambos archivos mapeando exactamente las columnas de Benchmark.cpp
    df_scal = pd.read_csv(filename_scal, sep=r'\s+', comment='#', skiprows=1,
                     names=['Sync', 'Sched', 'Chunk', 'Threads', 'Speedup', 
                            'Sigma_Sp', 'Serial_F', 'Sigma_SF', 'Theo_Sp', 
                            'Sigma_TSp', 'Eff', 'Sigma_Eff'])
                            
    df_bench = pd.read_csv(filename_bench, sep=r'\s+', comment='#', skiprows=1,
                      names=['Sync_b', 'Threads_b', 'MeanTime', 'StdDev'])
                      
    # 2. Unir los tiempos al dataframe principal (se escribieron en el mismo orden en C++)
    df = pd.concat([df_scal, df_bench[['MeanTime', 'StdDev']]], axis=1)
    
    # Limpiar y parsear
    df = df[pd.to_numeric(df['Threads'], errors='coerce').notnull()]
    df = df.astype({'Sched': int, 'Chunk': int, 'Threads': int, 'MeanTime': float, 'StdDev': float})
    
    # Filtrar para el máximo número de hilos evaluado
    max_hilos = df['Threads'].max()
    df_max = df[df['Threads'] == max_hilos]
    
    plt.figure(figsize=(10, 6))
    
    # Colores y marcadores similares al anexo de referencia
    schedules = {
        0: ('static', '#1f77b4', 'o'),     # Azul
        1: ('dynamic', '#ff7f0e', 's'),    # Naranja
        2: ('guided', '#2ca02c', '^')      # Verde
    }
    
    # 3. Graficar primero las líneas base (los "default" sin chunk)
    for sched_val, (label, color, marker) in schedules.items():
        data_sched = df_max[df_max['Sched'] == sched_val]
        
        # Buscar el valor por defecto (Chunk == 0)
        data_default = data_sched[data_sched['Chunk'] == 0]
        if not data_default.empty:
            mean_time = data_default['MeanTime'].values[0]
            plt.axhline(y=mean_time, color=color, linestyle='--', alpha=0.5, 
                        label=f'{label}_default (sin chunk)')

    # 4. Graficar las curvas con barras de error empírico (Chunk > 0)
    for sched_val, (label, color, marker) in schedules.items():
        data_sched = df_max[df_max['Sched'] == sched_val]
        
        data_chunks = data_sched[data_sched['Chunk'] > 0].sort_values('Chunk')
        if not data_chunks.empty:
            plt.errorbar(data_chunks['Chunk'], data_chunks['MeanTime'], yerr=data_chunks['StdDev'], 
                         label=label, color=color, marker=marker, capsize=4, linewidth=1.5)
                         
    # 5. Formato del gráfico
    plt.title('Tiempo vs chunk_size por schedule OpenMP', fontsize=12)
    plt.xlabel('chunk_size', fontsize=10)
    plt.ylabel('Tiempo medio (s)', fontsize=10)
    
    # Cambiar el eje X a escala logarítmica base 2
    ax = plt.gca()
    ax.set_xscale('log', base=2)
    
    # Generar las etiquetas dinámicas como potencias de 2 (Ej: 2^0, 2^1, etc.)
    chunks_presentes = df_max[df_max['Chunk'] > 0]['Chunk'].unique()
    if len(chunks_presentes) > 0:
        ax.set_xticks(chunks_presentes)
        ax.set_xticklabels([f'$2^{{{int(np.log2(c))}}}$' if c & (c-1) == 0 else str(c) for c in chunks_presentes])
    
    plt.legend(loc='upper left', fontsize=8)
    plt.grid(True, which="both", linestyle='-', alpha=0.3)
    
    output_file = 'chunks_plot.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"¡Éxito! Gráfico guardado como '{output_file}'")

except Exception as e:
    print(f"Error procesando los chunks: {e}")