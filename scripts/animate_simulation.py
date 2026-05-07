import matplotlib.pyplot as plt
import matplotlib.animation as animation
import pandas as pd
import numpy as np

# --- CONFIGURACIÓN DE VISIBILIDAD ---
PARTICULAS_A_MOSTRAR = 150  
SALTAR_FRAMES = 2           
FILENAME = "trajectories.dat"

try:
    print("Cargando y filtrando datos de trayectorias...")
    # Cargar datos (necesita pandas instalado en el Dockerfile)
    df = pd.read_csv(FILENAME, sep=r'\s+', comment='#', 
                     names=['step', 'id', 'mass', 'x', 'y', 'vx', 'vy'])

    # 1. Subsampling Temporal
    df = df[df['step'] % SALTAR_FRAMES == 0]

    # 2. Subsampling Espacial
    ids_unicos = df['id'].unique()
    ids_seleccionados = np.random.choice(ids_unicos, size=min(PARTICULAS_A_MOSTRAR, len(ids_unicos)), replace=False)
    df = df[df['id'].isin(ids_seleccionados)]

    # Configurar Gráfico
    fig, ax = plt.subplots(figsize=(8, 8), facecolor='white')
    ax.set_xlim(df['x'].min(), df['x'].max())
    ax.set_ylim(df['y'].min(), df['y'].max())
    ax.set_title(f"Dinámica de Partículas (Muestra: {PARTICULAS_A_MOSTRAR})", fontsize=14)
    
    # Cambiado a 'coolwarm' (Azul para lento, Rojo para rápido) y puntos más grandes
    scatter = ax.scatter([], [], c=[], cmap='coolwarm', s=25, edgecolors='black', linewidth=0.3)

    def update(frame):
        current = df[df['step'] == frame]
        scatter.set_offsets(np.column_stack((current['x'], current['y'])))
        v_mag = np.sqrt(current['vx']**2 + current['vy']**2)
        scatter.set_array(v_mag)
        return scatter,

    print("Generando video .mp4 de alta visibilidad...")
    steps = sorted(df['step'].unique())
    ani = animation.FuncAnimation(fig, update, frames=steps, blit=True)

    # El FFmpegWriter asegura que usemos el codec mp4 del contenedor
    writer = animation.FFMpegWriter(fps=30, bitrate=2000)
    ani.save("simulation_video.mp4", writer=writer)
    print("¡Éxito! Video guardado como simulation_video.mp4")

except Exception as e:
    print(f"Error en la visualización: {e}")