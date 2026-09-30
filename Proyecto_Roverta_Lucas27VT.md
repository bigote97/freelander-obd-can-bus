
# Proyecto Roverta - Investigación Lucas 27VT Freelander 1 MY99

## Vehículo objetivo

VIN: SALLNABB8XA690228

Decodificación:
- SAL: Land Rover UK
- LN: Freelander 1
- A: 5 puertas
- B: Motor L-Series 2.0 XEDi
- B: Manual 5 velocidades
- 8: Especificación de mercado
- X: Año modelo 1999
- A: Solihull
- 690228: Serie

## Objetivo del proyecto

Desarrollar una plataforma de diagnóstico y control para la CCU Lucas 27VT utilizando el puerto OBD, capaz de:

1. Detectar automáticamente parámetros de comunicación.
2. Leer configuración y estados internos.
3. Ejecutar pruebas de actuadores.
4. Detectar Transit Mode.
5. Desactivar Transit Mode.
6. Generar reportes CSV y logs técnicos.

---

# Hallazgos confirmados

## Arquitectura MY99

La investigación concluye que la camioneta corresponde a una arquitectura MY99 basada en:

- Lucas 27VT CCU
- Motor Rover L-Series XEDi
- K-Line ISO9141
- EKA disponible
- Configuración almacenada en CCU

## Capacidades conocidas de la CCU

Herramientas comerciales como PScan y Faultmate permiten:

- Leer EKA
- Leer configuración
- Programar mandos RF
- Leer estados
- Leer historial de alarma
- Ejecutar Output Tests
- Configurar parámetros internos

## Hipótesis Transit Mode

Los síntomas observados:

- Portón trasero sin funcionamiento normal
- Vidrio trasero sin funcionamiento normal
- Problemas de cierre centralizado
- Bosch ESI-Tronic puede abrir/cerrar

son compatibles con una configuración anómala o Transit Mode.

---

# Riesgos y contradicciones detectadas

## Baudrate

Fuentes encontradas:

- 9600 bps
- 10400 bps

Estado actual:
NO CONFIRMADO

El software deberá detectarlo dinámicamente.

## Dirección ECU

Posibles direcciones:

- 0x1C
- 0x9D
- 0x10
- 0x17

Estado actual:
NO CONFIRMADO

## EEPROM

Existen dos hipótesis:

A)
- EEPROM externa 93C46/93C66

B)
- EEPROM interna MCU Motorola

Estado actual:
NO CONFIRMADO

---

# Arquitectura propuesta V1

## Filosofía

No asumir ningún parámetro fijo.

El sistema deberá descubrir:

- velocidad
- dirección
- inicialización
- capacidades

automáticamente.

---

# Hardware V1

## Conexión al vehículo

Se propone utilizar:

VAG KKL 409.1 USB

Razones:

- económico
- sin licencias
- acceso directo a K-Line
- ampliamente disponible
- evita diseñar electrónica automotriz inicialmente

Arquitectura:

Freelander OBD
↓
KKL 409.1 USB
↓
Notebook

## Componentes

### Vehículo

- Puerto OBD original

### Interfaz

- KKL 409.1 USB
- FT232RL o CH340

### Notebook

- Windows/Linux
- Python 3

---

# Software V1

## Backend

Python

Librerías previstas:

- pyserial
- pandas
- FastAPI o Flask
- sqlite

## Frontend

Dashboard web local.

Características:

- detección ECU
- logs HEX
- export CSV
- reportes HTML

---

# Etapa 1 - Descubrimiento

## Objetivo

Descubrir automáticamente:

- baudrate
- dirección
- init sequence
- keywords
- checksum

## Flujo

1. Usuario conecta KKL.
2. Usuario presiona Scan.
3. Sistema prueba baudrates.
4. Sistema prueba métodos de inicialización.
5. Sistema prueba direcciones.
6. Detecta respuestas válidas.
7. Almacena resultados.

## Datos a mostrar

- Baudrate detectado
- Dirección detectada
- Keywords
- Serial ECU
- Hardware ID
- Software ID
- Coding Index
- Country
- Estado puertas
- Estado alarma
- Estado portón
- Estado luneta

## Exportación

Archivos:

- scan.csv
- raw_log.txt

---

# Etapa 2 - Output Tests

## Objetivo

Verificar control de actuadores.

## Pruebas previstas

### Unlock

CCU → apertura puertas

### Lock

CCU → cierre puertas

### Tailgate

Portón trasero

### Rear Window

Luneta trasera

### Hazard

Balizas

### Horn

Bocina

## Flujo

1. Usuario selecciona prueba.
2. Sistema ejecuta.
3. Usuario confirma resultado.
4. Se genera reporte.

---

# Etapa 3 - Transit Mode

## Detección

Buscar:

- Country = 0
- Transport Mode activo
- Configuraciones incompatibles

## Procedimiento

1. Leer configuración.
2. Crear backup.
3. Mostrar advertencia.
4. Confirmación usuario.
5. Escritura.
6. Verificación posterior.

## Objetivo

Country = 57 (Argentina)

si la investigación futura confirma dicho parámetro.

---

# Evolución futura

## V2

ESP32 + K-Line

Arquitectura:

OBD
↓
Transceptor K-Line
↓
ESP32
↓ WiFi
Notebook

## V3

Dispositivo autónomo:

- WiFi
- Bluetooth
- Dashboard integrado
- Control desde celular

---

# Gaps pendientes

1. Dirección exacta 27VT.
2. Baudrate real.
3. Algoritmo checksum.
4. Comandos output tests.
5. Comandos Transit Mode.
6. Layout EEPROM.
7. Secuencia completa de inicialización.

---

# Conclusión

La estrategia correcta es construir primero una plataforma de descubrimiento dinámica y de solo lectura utilizando un adaptador KKL USB y una notebook.

Una vez identificado el protocolo real de la Lucas 27VT MY99, evolucionar hacia un dispositivo dedicado basado en ESP32.
