# Informe Técnico de Investigación: Protocolo Lucas 27VT (Freelander 1 MY97-MY99)

Este informe detalla los hallazgos técnicos sobre la unidad de control central (CCU) Lucas 27VT, utilizada en los modelos Land Rover Freelander 1 de los años 1997 a 1999. La investigación se centra en el protocolo de diagnóstico, el modo de transporte (Transit Mode), la estructura de la memoria EEPROM y el hardware interno.

## 1. Protocolo de Diagnóstico Lucas 27VT (Prioridad 1)

El protocolo de diagnóstico de la Lucas 27VT se basa en una implementación específica de **K-Line (ISO 9141)**, muy similar al protocolo utilizado en las unidades Lucas 5AS y 10AS de la misma época.

### Parámetros de Comunicación
| Parámetro | Valor Identificado | Notas |
| --- | --- | --- |
| **Capa Física** | K-Line (Pin 7 del conector OBDII) | No utiliza línea L para comunicación bidireccional. |
| **Velocidad (Baudrate)** | **10400 bps** | Estándar para ISO 9141/KWP2000. |
| **Inicialización** | **5-Baud (Slow Init)** | Requiere un despertar de 5 baudios antes de subir a 10400 bps. |
| **Dirección ECU** | **0x1C** (Probable) | Basado en similitudes con la Lucas 5AS (0x1C para modo normal, 0x9D para modo EEPROM). |
| **Estructura de Mensajes** | Formato de bloque (Block Exchange) | Típico de los protocolos Lucas de los 90 (KWP-71/81). |
| **Checksum** | Suma de comprobación de 8 bits | Generalmente es la suma de todos los bytes del mensaje truncada a 8 bits (MOD 256). |

### Proceso de Inicialización (Hipótesis Técnica)
1. El tester envía el byte de dirección (**0x1C**) a una velocidad de **5 baudios**.
2. La CCU responde con un byte de sincronización (**0x55**) a **10400 baudios**.
3. Siguen dos "Keyword Bytes" (posiblemente **0x83 0x76** o similares) que identifican el protocolo.
4. El tester envía el complemento del último keyword byte para confirmar la conexión.

---

## 2. Transit Mode (Prioridad 2)

El **Transit Mode** (o Modo de Transporte) es un estado interno diseñado para reducir el consumo de batería durante el envío del vehículo desde la fábrica.

### Detalles del Almacenamiento y Desactivación
*   **Ubicación:** Se almacena como un valor de configuración en la **EEPROM**.
*   **Flag Identificado:** Según la documentación de Blackbox Solutions (SM024), el Transit Mode se activa cuando el **Código de País (Country Code)** se establece en **0**.
*   **Cómo se lee:** A través de la función "Settings" de herramientas como PScan o Faultmate.
*   **Cómo se desactiva:** Cambiando el código de país de **0** a cualquier código de país válido (por ejemplo, **2** para UK o **19** para España). Esto se realiza mediante una escritura diagnóstica en la EEPROM.

---

## 3. Output Tests y Funciones Diagnósticas (Prioridad 3)

La CCU 27VT permite el accionamiento de actuadores para pruebas de diagnóstico.

### Actuadores Identificados
*   **LOCK / UNLOCK:** Control de los motores de cierre centralizado.
*   **TAILGATE:** Accionamiento del cierre del portón trasero.
*   **REAR WINDOW:** Control de subida/bajada de la ventana trasera (crítico en Freelander para la apertura del portón).
*   **WIPERS / WASHERS:** Limpiaparabrisas delanteros y traseros.
*   **SOUNDERS:** Bocina de la alarma y zumbadores internos.
*   **LIGHTS:** Luces de cortesía, indicadores y LED de la alarma.

---

## 4. Estructura de la EEPROM (Prioridad 4)

La memoria EEPROM (probablemente una **93C46** o **93C66**) contiene la configuración crítica del vehículo.

### Mapa de Datos (Objetivos de Localización)
*   **VIN:** Almacenado en los últimos 6 dígitos del número de serie.
*   **EKA (Emergency Key Access):** Código de 4 dígitos para desactivar la alarma manualmente. En versiones MY97-MY99, este código es legible y gestionado directamente por la 27VT.
*   **Transit Mode:** Vinculado al byte de configuración de "Country".
*   **Mandos RF (Plips):** Almacena hasta 4 códigos de mandos Lucas 3TXB (433MHz o 315MHz según mercado).

---

## 5. Herramientas de Referencia (Prioridad 5)

| Herramienta | Capacidades Conocidas |
| --- | --- |
| **PScan.uk** | Lectura de EKA (97-99), programación de mandos, datos en vivo y actuadores. |
| **Blackbox Faultmate (SM024)** | Acceso completo a settings, cambio de país (Transit Mode) y sincronización EDC. |
| **TestBook / T4** | Herramienta original de concesionario, capaz de realizar todas las funciones, incluyendo el "Transit Mode removal". |

---

## 6. Relación con Lucas 5AS y Hardware (Prioridades 6 y 7)

### Similitudes con 5AS
La 27VT es esencialmente una evolución de la **5AS** con funciones de carrocería adicionales. Comparten el mismo esquema de comunicación K-Line y, muy probablemente, el mismo conjunto de comandos base para la gestión de la EEPROM.

### Hardware Interno
*   **Microcontrolador:** Familia **Motorola 68HC05** (probablemente una variante protegida).
*   **Transceptor K-Line:** Chip estándar como el **L9637D** o **Si9241**.
*   **Memoria:** EEPROM serie tipo **93C46** (1K bit).

---

## Conclusiones para el Proyecto
El éxito del proyecto depende de replicar la inicialización de 5 baudios con la dirección **0x1C**. Una vez establecida la sesión a 10400 bps, los comandos de lectura de EEPROM (probablemente siguiendo el estándar KWP-71) permitirán extraer el EKA y modificar el flag de Transit Mode cambiando el byte de "Country".
