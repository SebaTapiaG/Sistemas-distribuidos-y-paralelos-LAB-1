import subprocess
import os
import matplotlib.pyplot as plt
import matplotlib.image as mpimg

# 1. Lista exhaustiva de scripts modulares a ejecutar
scripts_to_run = [
    "scripts/plot_speedup_gpu.py",
    "scripts/plot_kernel_vs_endtoend.py",
    "scripts/plot_blockdim.py",
    "scripts/plot_amdahl.py",
    "scripts/plot_basic_vs_shared.py",
    "scripts/plot_energy.py"
]

print("=== INICIANDO GENERACIÓN DE GRÁFICOS ===")

# 2. Ejecutar cada script de Python individualmente
for script in scripts_to_run:
    try:
        print(f"Ejecutando {script}...")
        # Llama a Python para ejecutar el script y espera a que termine
        subprocess.run(["python3", script], check=True)
    except subprocess.CalledProcessError as e:
        print(f"[FALLO] Error al ejecutar {script}: {e}")
    except FileNotFoundError:
        print(f"[ERROR] No se encontró el script {script}")

print("=== CONSOLIDANDO DASHBOARD FINAL ===")

# 3. Lista estricta de las imágenes requeridas para el dashboard de rendimiento
imagenes = [
    'plot_speedup_gpu.png',
    'plot_kernel_vs_endtoend.png',
    'plot_blockdim.png',
    'plot_amdahl.png',
    'plot_basic_vs_shared.png'
]

# Verificar que todas las imágenes existen antes de armar el collage
if all(os.path.exists(img) for img in imagenes):
    # Creamos un panel de 2 filas por 3 columnas para acomodar los 5 gráficos
    fig, axes = plt.subplots(2, 3, figsize=(24, 12))
    
    # Aplanamos el arreglo de ejes para iterarlo fácilmente
    axes_flat = axes.flatten()
    
    # Cargar y mostrar cada imagen en su respectivo cuadrante
    for i, img_path in enumerate(imagenes):
        img = mpimg.imread(img_path)
        axes_flat[i].imshow(img)
        axes_flat[i].axis('off')  # Ocultar los ejes numéricos del contenedor
        
    # Ocultar el 6to panel (índice 5) que quedará vacío
    axes_flat[5].axis('off')
    
    plt.tight_layout()
    # Guardamos el archivo con el nombre exacto que exige el enunciado
    plt.savefig('performance_plots.png', dpi=300, bbox_inches='tight')
    print("¡Éxito! Dashboard final consolidado como 'performance_plots.png'")
else:
    print("[ADVERTENCIA] Faltan algunas imágenes individuales. Asegúrate de tener los archivos .dat para generar 'performance_plots.png'")