import matplotlib.pyplot as plt

filename = "scaling_analysis.dat"
hilos, eficiencia, sigma_eff = [], [], []

try:
    with open(filename, 'r') as f:
        for line in f:
            if line.startswith('#') or line.strip() == "":
                continue
            
            valores = line.split()
            if len(valores) >= 8:
                hilos.append(int(valores[1]))          # Columna 1: Threads
                eficiencia.append(float(valores[6]))   # Columna 6: Eficiencia
                sigma_eff.append(float(valores[7]))    # Columna 7: Error de Eficiencia
    
    plt.figure(figsize=(10, 6))
    
    # Línea ideal: Eficiencia perfecta es 1.0 (100%)
    plt.axhline(y=1.0, color='gray', linestyle='--', label='Eficiencia Ideal (1.0)')
    
    # Curva real
    plt.errorbar(hilos, eficiencia, yerr=sigma_eff, label='Eficiencia Real Medida', 
                 color='green', marker='o', capsize=5, linewidth=2)
    
    plt.title('Análisis de Eficiencia Paralela', fontsize=14)
    plt.xlabel('Número de Hilos', fontsize=12)
    plt.ylabel('Eficiencia (Speedup / Hilos)', fontsize=12)
    plt.xticks(hilos)
    plt.ylim(0, 1.2) # La eficiencia rara vez supera 1.0
    
    plt.legend()
    plt.grid(True, linestyle=':', alpha=0.7)
    
    output_file = 'efficiency_plot.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"¡Éxito! Gráfico guardado como '{output_file}'")

except FileNotFoundError:
    print(f"Error: No se encontró '{filename}'.")