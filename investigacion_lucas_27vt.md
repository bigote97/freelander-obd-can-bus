# Plan de Investigación e Ingeniería Inversa: Lucas 27VT CCU
**Vehículo de Referencia:** Land Rover Freelander 1 XEDi (1999)
**Chasis:** SALLNABB8XA690228

---

## ⚙️ Prioridad 1: Protocolo de Diagnóstico Lucas 27VT (CRÍTICO)

La unidad de control central (CCU) Lucas 27VT fue desarrollada en la transición previa a la estandarización estricta de OBD2 en Europa. Aunque utiliza el conector físico J1962 (OBDII), el protocolo de comunicación es propietario.

### Especificaciones Técnicas Estimadas
* **Capa Física:** K-Line dedicada. Físicamente basada en las especificaciones eléctricas de la norma ISO-9141 / ISO-14230. En el ecosistema Land Rover/Rover de esta época, la línea de diagnóstico de carrocería suele direccionarse a un pin específico del conector OBD (frecuentemente el Pin 7 o Pin 8, compartido o adyacente a la línea de la alarma/inmovilizador).
* **Velocidad de Transmisión (Baudrate):** 10400 bps (estándar ISO de la época) o 9600 bps.
* **Secuencia de Inicialización (Wake-up):**
  * **5-Baud Init:** Envío de una palabra de dirección específica a una velocidad extremadamente baja (5 baudios) para "despertar" a la ECU, la cual responde enviando bytes de sincronización (Keywords) para conmutar la línea a 10400 bps.
  * **Fast Init:** Mantener la línea K en estado lógico bajo (Low) durante 25 milisegundos, seguido de un tiempo en alto (High) antes de enviar el primer byte de comando a 10400 bps.
* **Dirección Física de la ECU (Target Address):** En la arquitectura Rover/Lucas, los módulos de carrocería y sistemas de seguridad responden típicamente a las direcciones hexadecimales `$10` o `$17`.
* **Estructura de Mensajes (Keyword Protocol 82 / KWP2000 primitivo):**
  $$\text{[Header/Length]} + \text{[Target Address]} + \text{[Source Address]} + \text{[Service ID (SID)]} + \text{[Data Bytes...]} + \text{[Checksum]}$$
* **Suma de Comprobación (Checksum):** Checksum de 8 bits simple, calculado como la suma modular de todos los bytes del mensaje (módulo 256).

---

## 🚚 Prioridad 2: Transit Mode (Modo de Transporte)

El *Transit Mode* es un estado de software implementado en fábrica para mitigar la descarga de la batería durante el transporte marítimo y almacenamiento del vehículo. Desactiva funciones de confort como el receptor de radiofrecuencia (RF), retardos de luces de cortesía y alarmas perimetrales.

### Mecanismo de Funcionamiento
* **Almacenamiento:** Se almacena como un indicador de estado (*flag* de 1 byte o 1 bit) en una celda de memoria no volátil (EEPROM).
* **Lectura/Diagnóstico:** Herramientas oficiales como el *Land Rover TestBook (T4)* o sistemas avanzados como el *Faultmate (módulo SM024)* leen este estado mediante solicitudes de diagnóstico específicas independientes de los códigos de falla (DTC).
* **Desactivación por Software:** Se ejecuta mediante un comando de servicio del tipo *Write Data By Local Identifier* o comandos dedicados de configuración que sobrescriben el byte correspondiente (ej. cambiar el estado de `$01` a `$00`).
* **Modificación por Hardware:** En lecturas directas del mapa de memoria (dump), la desactivación se evidencia en la alteración de un único byte específico de configuración del sistema.

---

## 🔒 Prioridad 3: Pruebas de Actuadores (Output Tests)

La CCU controla directamente las cargas de potencia de la carrocería mediante drivers de baja potencia que comandan relés físicos. El protocolo expone servicios de control de actuadores equivalentes al estándar *Input/Output Control by Local Identifier*.

### Funciones Diagnósticas Clave
* **LOCK / UNLOCK:** Activación de los relés de inversión de polaridad para los motores de las cerraduras de las 5 puertas.
* **TAILGATE (Portón Trasero):** Liberación del pestillo eléctrico del portón trasero.
* **REAR WINDOW (Luneta Trasera):** Activación del motor de la luneta para descender los centímetros mandatorios que permiten la apertura segura del portón, un mecanismo crítico en la Freelander 1.

*Nota:* El disparo de estas salidas se realiza enviando el identificador de servicio de actuación seguido del identificador del componente y los parámetros de control (ej. tiempo de activación o estado activo/inactivo).

---

## 🧠 Prioridades 4 y 7: Arquitectura de Hardware y EEPROM

El desmontaje y análisis físico de la placa de circuito impreso (PCB) de la CCU 27VT revela una arquitectura compartida con los sistemas de seguridad contemporáneos de Lucas.

### Componentes Críticos del Hardware
* **Microcontrolador (MCU):** Utiliza un microcontrolador de la familia **Motorola HC05** (comúnmente variantes como el `MC68HC05B16`) o **Motorola HC11**.
* **Memoria EEPROM:** No existe un chip EEPROM externo integrado de 8 pines (como las series 24Cxx o 93Cxx). La memoria de configuración, el **VIN**, el código de acceso de emergencia (**EKA**), el mapa de los mandos RF y el flag de *Transit Mode* se encuentran **dentro del área de EEPROM interna del propio microcontrolador**.
* **Interfaz de Línea K:** Implementada mediante transceptores de línea específicos (estilo ISO K-Line drivers o arreglos de transistores discretos) encargados de adaptar los niveles lógicos del microcontrolador (0-5V) a los niveles de tensión de la batería del vehículo (0-12V).

### Desafío de Seguridad para la Lectura (Dump)
Los microcontroladores programados por Lucas poseen activo el **Security Bit** (Bit de Seguridad). Los intentos de lectura convencionales a través de interfaces de programación estándar provocarán el bloqueo del microcontrolador o la entrega de datos corruptos. La extracción limpia de la memoria requiere programadores especializados en automoción (como **XPROG-M**, **VVDI Prog** o herramientas analógicas de *glitching*) que manipulen las señales de reloj y voltaje para omitir la rutina de protección de la memoria.

---

## 🛠️ Prioridades 5 y 6: Herramientas del Mercado y Relación con Lucas 5AS

El desarrollo de software propio puede acelerarse analizando las soluciones comerciales existentes y la documentación de sistemas hermanos de la misma época.

### Soluciones de Diagnóstico Comerciales
* **PScan (pscan.uk):** Esta herramienta de diagnóstico independiente descifró por completo la estructura de comandos de la CCU 27VT para los modelos MY97-MY99. Es capaz de extraer el código EKA y realizar adaptaciones de telemandos a través de la línea K, confirmando que el protocolo de seguridad en estos años no requiere validaciones por servidores externos.
* **Faultmate (Blackbox Solutions):** El módulo **SM024** expone todas las funciones de desarrollo de ingeniería para la 27VT, incluyendo la lectura de la memoria interna, estados de alarma históricos, desactivación de modos de transporte y pruebas dinámicas de actuadores.

### Ingeniería Inversa Comparativa: El Ecosistema Lucas 5AS
La unidad de inmovilizador **Lucas 5AS** (utilizada extensamente en los Rover 200/400/25/45, MG ZR/ZS, y Caterham) es contemporánea y comparte el mismo núcleo tecnológico y equipo de diseño de ingeniería que la CCU 27VT de la Freelander.
* **Valor Estratégico:** El protocolo de la Lucas 5AS ha sido completamente sometido a ingeniería inversa por comunidades de código abierto (existiendo librerías en GitHub, scripts de Python y esquemáticos detallados).
* **Paralelismo Técnico:** La estructura de sincronización, las tramas de hermanado entre el inmovilizador y la ECU de motor, el algoritmo de cifrado para los mandos de radiofrecuencia (Keeloq de primera generación) y la velocidad de la K-Line son prácticamente idénticos en ambos módulos. Utilizar la documentación pública de la 5AS proporciona el marco teórico y estructural para programar la comunicación con la 27VT.
