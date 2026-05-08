import matplotlib.pyplot as plt
import pandas as pd

filename = "scaling_analysis.dat"

try:
    print("Generando análisis de balanceo de carga (Chunks)...")
    df = pd.read_csv(filename, sep=r'\s+', comment='#', skiprows=1,
                     names=['Sync', 'Sched', 'Chunk', 'Threads', 'Speedup', 
                            'Sigma_Sp', 'Serial_F', 'Sigma_SF', 'Theo_Sp', 
                            'Sigma_TSp', 'Eff', 'Sigma_Eff'])
    
    # Excluir líneas de encabezados repetidos si las hay y forzar numérico
    df = df[pd.to_numeric(df['Threads'], errors='coerce').notnull()]
    df = df.astype({'Sched': int, 'Chunk': int, 'Threads': int, 'Speedup': float, 'Sigma_Sp': float})

    # Filtrar solo la ejecución a máximo rendimiento (Ej: 8 hilos)
    max_hilos = df['Threads'].max()
    df_max = df[df['Threads'] == max_hilos]

    plt.figure(figsize=(10, 6))
    
    # 1=Dynamic, 2=Guided
    schedules = {1: ('Dynamic', 'blue', 'o'), 2: ('Guided', 'green', 's')}
    
    for sched_val, (label, color, marker) in schedules.items():
        data = df_max[df_max['Sched'] == sched_val].sort_values('Chunk')
        if not data.empty:
            plt.errorbar(data['Chunk'], data['Speedup'], yerr=data['Sigma_Sp'], 
                         label=f'{label} Schedule', color=color, marker=marker, 
                         capsize=5, linewidth=2)

    # El caso Static (Sched=0) como línea base roja
    static_data = df_max[df_max['Sched'] == 0]
    if not static_data.empty:
        static_sp = static_data['Speedup'].mean()
        plt.axhline(y=static_sp, color='red', linestyle='--', 
                    label=f'Static Baseline (Sp={static_sp:.2f})')

    plt.title(f'Impacto del Balanceo de Carga en OpenMP ({max_hilos} Hilos)', fontsize=14)
    plt.xlabel('Tamaño del Chunk', fontsize=12)
    plt.ylabel('Speedup Real Medido', fontsize=12)
    plt.legend()
    plt.grid(True, linestyle=':', alpha=0.7)
    
    output_file = 'chunks_plot.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"¡Éxito! Gráfico guardado como '{output_file}'")

except Exception as e:
    print(f"Error procesando los chunks: {e}")