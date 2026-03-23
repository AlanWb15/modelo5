from flask import Flask, render_template, request, redirect, url_for, flash
import MySQLdb

app = Flask(__name__)
app.secret_key = "supersecreto"  # Necesario para mensajes flash

# Configuración de la base de datos
db_config = {
    "host": "localhost",
    "user": "root",
    "passwd": "",       # tu contraseña de MySQL
    "db": "mi_base_datos"
}

@app.route('/')
def index():
    return render_template('login.html')

@app.route('/login', methods=['POST'])
def login():
    usuario = request.form['usuario']
    password = request.form['password']

    # Conectar a la base de datos
    conn = MySQLdb.connect(**db_config)
    cursor = conn.cursor()

    # Evitar inyección SQL
    cursor.execute("SELECT * FROM usuarios WHERE usuario=%s AND password=%s", (usuario, password))
    account = cursor.fetchone()

    cursor.close()
    conn.close()

    if account:
        return "¡Login exitoso!"
    else:
        return "Usuario o contraseña incorrectos."

if __name__ == '__main__':
    app.run(debug=True)