import shutil
import time
import os

# Carpeta origen (MT5)
origen = r"C:\Users\danyj\AppData\Roaming\MetaQuotes\Terminal\Common\Files"

# Carpeta destino (donde está este .py)
destino = os.path.dirname(os.path.abspath(__file__))

archivo_nombre = None

# Detecta automáticamente el CSV
for f in os.listdir(origen):
    if f.endswith(".csv"):
        archivo_nombre = f
        break

if not archivo_nombre:
    print("No se encontró CSV")
    exit()

archivo_origen = os.path.join(origen, archivo_nombre)
archivo_destino = os.path.join(destino, archivo_nombre)

print("Copiando:", archivo_nombre)

while True:
    try:
        shutil.copy2(archivo_origen, archivo_destino)
    except:
        pass
    time.sleep(1)

#python -m http.server 8000
