import matplotlib.pyplot as plt
import matplotlib.animation as animation
import pandas as pd
import numpy as np

# --- CONFIGURACIÓN DE VISIBILIDAD ---
PARTICULAS_A_MOSTRAR = 150
SALTAR_FRAMES = 5    
LARGO_ESTELA = 15    
FILENAME = "trajectories.dat"

try:
    print("Cargando y filtrando datos de trayectorias...")
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
    
    scatter = ax.scatter([], [], c=[], cmap='coolwarm', s=25, edgecolors='black', linewidth=0.3, zorder=3)
    
    
    trail_lines = {}
    for p_id in ids_seleccionados:
        # Se crean líneas delgadas y semi-transparentes
        line, = ax.plot([], [], '-', color='gray', alpha=0.3, linewidth=1.2, zorder=2)
        trail_lines[p_id] = line
        
    def update(frame):
        current = df[df['step'] == frame]
        scatter.set_offsets(np.column_stack((current['x'], current['y'])))
        v_mag = np.sqrt(current['vx']**2 + current['vy']**2)
        scatter.set_array(v_mag)
        
       
        # Filtramos los pasos anteriores hasta alcanzar el LARGO_ESTELA
        frame_minimo = max(0, frame - (LARGO_ESTELA * SALTAR_FRAMES))
        rango_frames = df[(df['step'] <= frame) & (df['step'] >= frame_minimo)]
        
        for p_id in ids_seleccionados:
            p_data = rango_frames[rango_frames['id'] == p_id]
            trail_lines[p_id].set_data(p_data['x'], p_data['y'])
            
        return [scatter] + list(trail_lines.values())
        
    print("Generando video .mp4 con estelas de movimiento...")
    steps = sorted(df['step'].unique())
    ani = animation.FuncAnimation(fig, update, frames=steps, blit=True)
    writer = animation.FFMpegWriter(fps=30, bitrate=2000)
    ani.save("simulation_video.mp4", writer=writer)
    print("¡Éxito! Video guardado como simulation_video.mp4")

except Exception as e:
    print(f"Error en la visualización: {e}")