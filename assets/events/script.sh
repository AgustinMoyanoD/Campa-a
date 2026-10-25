#!/bin/bash

# Validar que se hayan proporcionado los argumentos necesarios
if [ "$#" -lt 2 ]; then
    echo "Uso: $0 <directorio> <valor_X>"
    echo "Ejemplo: $0 /ruta/a/carpeta 2"
    echo "Nota: <valor_X> debe ser 1, 2 o 4."
    exit 1
fi

DIRECTORIO="$1"
X="$2"

# Validar que X sea exactamente 1, 2 o 4
if [[ "$X" != "1" && "$X" != "2" && "$X" != "4" ]]; then
    echo "Error: El valor de X ('$X') no es válido. Debe ser 1, 2 o 4."
    exit 1
fi

# Verificar si el directorio existe
if [ ! -d "$DIRECTORIO" ]; then
    echo "Error: El directorio '$DIRECTORIO' no existe."
    exit 1
fi

echo "Procesando archivos en '$DIRECTORIO' con Probability=$X..."

# Buscar archivos recursivamente y reemplazar el texto
find "$DIRECTORIO" -type f | while read -r archivo; do
    # Usamos sed para reemplazar todas las ocurrencias en cada archivo
    sed -i "s/Probability=2/Probability=$X/g" "$archivo"
done

echo "¡Proceso completado con éxito!"