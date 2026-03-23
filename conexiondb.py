from flask import Flask, render_template, request
import pandas as pd
from sqlalchemy import create_engine
import MySQLdb

app = Flask(__name__)
app.secret_key = "supersecreto"

# Configuración de la base de datos MySQL
DB_USER = 'root'
DB_PASSWORD = ''
DB_HOST = 'localhost'
DB_NAME = 'mi_base_datos'

# Crear engine de SQLAlchemy
engine = create_engine(f'mysql+mysqlconnector://{DB_USER}:{DB_PASSWORD}@{DB_HOST}/{DB_NAME}')

@app.route('/')
def index():
    return render_template('login.html')

# Login de usuarios
@app.route('/login', methods=['POST'])
def login():
    usuario = request.form['usuario']
    password = request.form['password']

    conn = MySQLdb.connect(host=DB_HOST, user=DB_USER, passwd=DB_PASSWORD, db=DB_NAME)
    cursor = conn.cursor()

    # Consulta segura usando placeholders
    cursor.execute("SELECT * FROM usuarios WHERE usuario=%s AND password=%s", (usuario, password))
    account = cursor.fetchone()

    cursor.close()
    conn.close()

    if account:
        return "¡Login exitoso!"
    else:
        return "Usuario o contraseña incorrectos."

# Importar datos desde Excel
@app.route('/importar', methods=['POST'])
def importar_excel():
    if 'excel_file' not in request.files:
        return "No se subió ningún archivo."

    file = request.files['excel_file']

    # Leer Excel con pandas
    df = pd.read_excel(file)

    # Insertar en la tabla 'usuarios' de la DB
    df.to_sql(name='usuarios', con=engine, if_exists='append', index=False)

    return "Datos importados correctamente."

if __name__ == '__main__':
    app.run(debug=True)
