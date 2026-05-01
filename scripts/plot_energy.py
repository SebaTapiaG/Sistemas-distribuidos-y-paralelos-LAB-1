import matplotlib.pyplot as plt

# 1. Leer los datos
filename = "energy_timeseries.dat"
pasos, k, u, e_total = [], [], [], []

try:
    with open(filename, 'r' ) as f:
        next(f) # Saltar la primera línea de cabeceras
        for line in f:
            valores = line.split()
            if len(valores) == 4:
                pasos.append(int(valores[0]))
                k.append(float(valores[1]))
                u.append(float(valores[2]))
                e_total.append(float(valores[3]))

    # 2. Configurar el gráfico
    plt.figure(figsize=(10, 6))
    
    # Graficar las tres curvas
    plt.plot(pasos, k, label='Energía Cinética (K)', color='blue', linewidth=2)
    plt.plot(pasos, u, label='Energía Potencial (U)', color='red', linewidth=2)
    plt.plot(pasos, e_total, label='Energía Total (E)', color='black', linewidth=2, linestyle='--')

    # 3. Darle formato profesional para el informe
    plt.title('Conservación de Energía a lo largo de la Simulación', fontsize=14)
    plt.xlabel('Paso de tiempo (Iteración)', fontsize=12)
    plt.ylabel('Energía (Joules / Unidades de simulación)', fontsize=12)
    plt.legend()
    plt.grid(True, linestyle=':', alpha=0.7)
    
    # 4. Guardar como PNG estático
    output_file = 'energy_plot.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"¡Éxito! Gráfico guardado como '{output_file}'")

except FileNotFoundError:
    print(f"Error: No se encontró el archivo '{filename}'.")
except Exception as e:
    print(f"Ocurrió un error inesperado: {e}")