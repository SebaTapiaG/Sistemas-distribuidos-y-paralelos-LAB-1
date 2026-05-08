import matplotlib.pyplot as plt

# 1. Leer los datos
filename = "scaling_analysis.dat"
hilos, speedup, sigma_sp, theo_sp = [], [], [], []

try:
    with open(filename, 'r') as f:
        for line in f:
            if line.startswith('#') or line.strip() == "" or line.startswith('Sync'):
                continue
            
            valores = line.split()
            if len(valores) >= 12:
                sched = int(valores[1])
                # Filtramos SOLO el caso base (Static)
                if sched == 0:
                    hilos.append(int(valores[3]))         # Threads
                    speedup.append(float(valores[4]))     # Speedup
                    sigma_sp.append(float(valores[5]))    # Error
                    theo_sp.append(float(valores[8]))
    
    # 2. Configurar el gráfico
    plt.figure(figsize=(10, 6))
    
    # Curva 1: La línea Ideal (si 4 hilos = 4x de velocidad)
    plt.plot(hilos, hilos, label='Speedup Ideal (Lineal)', color='gray', linestyle='--')
    
    # Curva 2: El Límite Matemático de tus compañeros (Ley de Amdahl)
    plt.plot(hilos, theo_sp, label='Límite Teórico (Amdahl)', color='red', linestyle='-.', marker='s')
    
    # Curva 3: El Rendimiento Real del Servidor (Con barras de error)
    plt.errorbar(hilos, speedup, yerr=sigma_sp, label='Speedup Real Medido', 
                 color='blue', marker='o', capsize=5, linewidth=2)
    
    # 3. Formato profesional
    plt.title('Análisis de Escalabilidad: Speedup vs Número de Hilos', fontsize=14)
    plt.xlabel('Número de Hilos', fontsize=12)
    plt.ylabel('Speedup (Aceleración)', fontsize=12)
    
    # Forzar que el eje X muestre exactamente 1, 2, 4, 8...
    plt.xticks(hilos) 
    
    plt.legend()
    plt.grid(True, linestyle=':', alpha=0.7)
    
    # 4. Guardar gráfico
    output_file = 'scalability_plot.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"¡Éxito! Gráfico guardado como '{output_file}'")

except FileNotFoundError:
    print(f"Error: No se encontró el archivo '{filename}'. Asegúrate de correr el benchmark primero.")
except Exception as e:
    print(f"Ocurrió un error inesperado: {e}")