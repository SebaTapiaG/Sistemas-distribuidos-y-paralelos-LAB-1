import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

filename = "blockdim_study.dat"

try:
    print(f"Leyendo datos desde {filename}...")
    # Ahora leemos el archivo permitiendo que Pandas infiera las columnas desde la línea 1
    df = pd.read_csv(filename, sep=r'\s+')
    
    # Obtener todos los valores únicos de N simulados
    n_values = df['N'].unique()
    num_plots = len(n_values)
    
    # Crear una figura dinámica dependiendo de la cantidad de tamaños N
    fig, axes = plt.subplots(1, num_plots, figsize=(5 * num_plots, 6))
    
    # Asegurarnos de que axes sea iterable en caso de que solo haya un N
    if num_plots == 1:
        axes = [axes]
        
    bar_width = 0.35
    
    # Generar un subgráfico por cada valor de N
    for i, n in enumerate(n_values):
        ax = axes[i]
        df_n = df[df['N'] == n]
        
        var0 = df_n[df_n['Variant'] == 0].sort_values('BlockSize')
        var1 = df_n[df_n['Variant'] == 1].sort_values('BlockSize')
        
        index = np.arange(len(var0['BlockSize']))
        
        ax.bar(index, var0['GpuKernel_ms'], bar_width, color='royalblue', 
               label='Kernel Básico', yerr=var0['KernelStdDev'], capsize=5)
        ax.bar(index + bar_width, var1['GpuKernel_ms'], bar_width, color='forestgreen', 
               label='Shared Memory', yerr=var1['KernelStdDev'], capsize=5)
        
        ax.set_title(f"N = {n}", fontsize=14, fontweight='bold')
        ax.set_xlabel('blockDim.x (Hilos)', fontsize=12)
        
        # Solo mostrar la etiqueta del eje Y en el primer gráfico para no saturar
        if i == 0:
            ax.set_ylabel('Tiempo Kernel-Only (ms)', fontsize=12)
            ax.legend()
            
        ax.set_xticks(index + bar_width / 2)
        ax.set_xticklabels(var0['BlockSize'])
        ax.grid(axis='y', linestyle=':', alpha=0.7)
    
    # Título principal de la figura completa
    plt.suptitle('Tiempo de Ejecución de Kernels vs blockDim.x', fontsize=18, y=1.05)
    
    plt.tight_layout()
    plt.savefig('plot_blockdim.png', dpi=300, bbox_inches='tight')
    print("¡Éxito! Gráfico consolidado guardado como plot_blockdim.png")

except Exception as e:
    print(f"Error procesando los datos: {e}")