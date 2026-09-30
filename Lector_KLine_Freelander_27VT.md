# Lector K-Line — Freelander 1 MY99, CCU Lucas 27VT

Documento técnico del dispositivo de campo. El ESP32 habla con la camioneta por el conector OBD. La Raspberry Pi Zero W registra la sesión. El firmware actual solo abre la comunicación y anota la respuesta. No escribe configuración.

Archivos:

| Pieza | Ruta |
| --- | --- |
| Firmware ESP32 | `firmware/esp32_kline/esp32_kline.ino` |
| Registro en la Pi o en la PC | `host/kline_log.py` |
| Logs | `host/logs/kline-AAAAMMDD-HHMMSS.log` |

## Vehículo

| Dato | Valor |
| --- | --- |
| VIN | `SALLNABB8XA690228` |
| Marca / planta | Land Rover, Solihull |
| Modelo | Freelander 1, 5 puertas |
| Año modelo | 1999 (código VIN `X`) |
| Motor | Rover L-Series 2.0 XEDi, turbodiésel |
| Caja | Manual 5 velocidades, volante a la izquierda |
| Unidad de carrocería | Lucas 27VT (CCU) |
| Coding Index esperado | 1 (EKA presente, inmovilizador integrado en la CCU) |
| ECU de motor | EDC del L-Series. Se reconoce con la CCU mediante el campo EDC code |

## Síntoma que motiva el lector

El botón del portón llega a la CCU: en el escáner Bosch la entrada cambia de estado. El vidrio y el pestillo no se mueven.

Ese patrón coincide con el Transit Mode descrito en el manual de taller de la Freelander. Con la CCU en ese modo quedan desactivados:

- receptor de radio de los mandos
- pestillo del portón
- luneta trasera
- cierre centralizado
- luces interiores

La CCU sigue despierta e informa el botón. Con contacto en posición II, el zumbador interno suena para avisar que el modo sigue activo.

En el Faultmate SM024 v1.22 el Transit Mode es el campo **Country = 0**. Cualquier otro código de país no cambia funciones: queda solo como etiqueta de mercado. Argentina es el código **57**. El valor **1** figura en la lista con la etiqueta "Transit" y no es el destino de la reparación.

En Coding Index 1 existe además un campo separado, **Transport mode**. Se lee junto con Country. La salida de fábrica, hecha con TestBook, T4, PScan o SM024, consiste en pasar Country de 0 a 57 y dejar el resto del bloque como estaba.

Los marcadores de temperatura y combustible no pasan por la 27VT. Van del sensor al tablero. Este dispositivo no los corrige.

## Bus de diagnóstico

La 27VT de este año modelo se alcanza por K-Line ISO 9141 en el pin 7 del conector de 16 pines. No hay sesión de diagnóstico de la CCU por CAN. Un transceptor CAN (MCP2515 u otro) no despierta esta unidad.

El mismo pin 7 lo comparten varios módulos del vehículo. Cada uno responde a su dirección de despertar. Una dirección que no corresponde produce silencio, sin escribir nada.

| Parámetro | Estado |
| --- | --- |
| Capa física | K-Line, un hilo, niveles de batería (0 V dominante, 12 V recesivo) |
| Conector | SAE J1962, zócalo del vehículo |
| Pin de datos | 7 |
| Masa de señal | Pin 4 (masa de chasis). El pin 5 es masa de señal desde el año modelo 2002.5 |
| Alimentación del zócalo | Pin 16, batería permanente. No se usa para alimentar el ESP32 |
| Pin 13 | Bus DS2 en la arquitectura de manual. En MY99 el inmovilizador está dentro de la 27VT, no en un módulo EWS separado |
| Inicialización | Lenta, 5 baudios, y después la velocidad de trabajo |
| Formato de byte | 8N1 mientras no aparezca otra cosa en una captura real |
| Velocidad de trabajo | 9600 o 10400 baudios. Las dos cifras circulan en la investigación y ninguna está medida en esta camioneta |
| Dirección de la 27VT | Sin confirmar. Candidatas de la investigación: `0x1C`, `0x10`, `0x17`. `0x33` es la dirección genérica ISO 9141 |
| Checksum y comandos de ajustes | Sin confirmar. PScan y SM024 los implementan en software cerrado |

La dirección `0x9D` despierta el modo de memoria en el Lucas 5AS, un módulo de la misma época. El firmware se niega a enviarla. No hay evidencia publicada de que en la 27VT haga lo mismo, y un disparo de ese tipo puede acercarse al área donde vive el código EDC.

## Reparto de placas

| Placa | Papel |
| --- | --- |
| ESP32 | Bit-bang de los 5 baudios y de la respuesta a ~10 kbaud. USB serie a 115200 hacia la computadora |
| Raspberry Pi Zero W | Computadora de campo. Corre `kline_log.py` y guarda el log. Puede alimentar el ESP32 por USB si la corriente alcanza |
| NodeMCU ESP8266 | No usado. El UART y el bit-bang a 10400 no son estables para este apretón de tiempos |
| Arduino Uno | No usado. El único UART de hardware es el del USB |

El apretón de tiempos lo hace el ESP32 en local. La Pi no puede cumplir la ventana de 25 a 50 ms del reconocimiento ISO si el diálogo viaja por Python.

GPIO usados en el ESP32 Dev Module:

| Señal | GPIO | Nota |
| --- | --- | --- |
| TX hacia el transistor o el L9637 | 25 | No es pin de arranque |
| RX desde el divisor | 26 | Entrada |
| USB hacia la Pi o la PC | UART0 del puente USB | 115200 8N1 |

Durante el arranque del ESP32 el GPIO 25 queda en alta impedancia. La resistencia de 100 kΩ entre base y masa mantiene el NPN apagado hasta que el sketch fuerza la línea a reposo.

## Interfaz eléctrica

El GPIO del ESP32 es de 3,3 V y no tolera 5 V ni 12 V. La K-Line en reposo está a tensión de batería, entre unos 11 V y 14,4 V con el alternador. El pin 7 entra solo a través del circuito de abajo.

### Circuito NPN (el de campo)

Transistor: 2N2222, BC547 o NPN equivalente. Emisor a masa.

```
OBD pin 4  ------------------------- GND ESP32
OBD pin 7  ----+---- 150 kΩ ---- GPIO26 (RX)
               |                    |
               |                  zener 3,3 V
               |                  cátodo en GPIO26, ánodo a GND
               |                    |
               |                  47 kΩ
               |                    |
               |                   GND
               |
               +---- 1 kΩ ---- colector NPN

GPIO25 ---- 1 kΩ ---- base NPN
base   ---- 100 kΩ --- GND
emisor ---------------- GND
```

El firmware arranca en modo `IFACE NPN`: GPIO en alto satura el transistor y pone la K-Line a masa (dominante). GPIO en bajo corta el transistor y la línea vuelve a 12 V por la resistencia de pull-up que ya tienen las ECU.

| Rama | Función | Cifra de diseño |
| --- | --- | --- |
| 1 kΩ de base | Limita la corriente de base | Con 3,3 V y Vbe ≈ 0,7 V, Ib ≈ 2,6 mA |
| 100 kΩ base-emisor | Apaga el NPN mientras el ESP32 arranca | Evita clavar la K-Line en dominante al conectar el USB |
| 1 kΩ de colector | Limita la corriente al poner la línea a masa | Protege transistor y drivers de las ECU si el firmware se queda en dominante |
| 150 kΩ + 47 kΩ | Divisor de lectura | A 14,4 V, sin zener, el nodo queda cerca de 3,4 V |
| Zener 3,3 V | Techo del GPIO26 | Con 14,4 V la corriente por los 150 kΩ es del orden de 70 µA |

Sin zener, 14 V en ese divisor se acerca al máximo del ESP32. El zener forma parte del circuito.

Alimentación del ESP32: USB de la Pi o power bank. El pin 16 del OBD no se conecta a 5 V, a 3,3 V ni a un GPIO.

Masa común obligatoria: pin 4 del OBD, GND del ESP32 y GND de la Pi.

### L9637, si aparece el integrado

El comando `IFACE L9637` trata la interfaz como no inversora: GPIO en alto deja la K-Line suelta. El VCC lógico del L9637 va a 3,3 V. Con VCC a 5 V, la pata RX entrega 5 V y daña el ESP32. La pata de batería del integrado, si se usa, sale del pin 16 a través de la red que indique su hoja de datos, nunca hacia un GPIO.

## Zócalo, visto de frente

Fila ancha arriba, pines 1 a 8 de izquierda a derecha. Fila angosta abajo, pines 9 a 16.

| Pin | Uso en este trabajo |
| --- | --- |
| 4 | Masa. Único retorno del lector |
| 7 | K-Line. Única señal |
| 16 | Batería. Se identifica para no usarlo. Esquina inferior derecha |

## Firmware

Sketch: `firmware/esp32_kline/esp32_kline.ino`.

Se carga con el IDE de Arduino, placa "ESP32 Dev Module", monitor serie a **115200** baudios. El sketch no usa WiFi ni Bluetooth: el bit-bang no puede compartir el tiempo con la radio.

### Comandos

| Comando | Efecto |
| --- | --- |
| `HELP` o `?` | Lista de comandos |
| `IFACE NPN` | Transistor inversor. Es el valor de arranque |
| `IFACE L9637` | Transceptor no inversor, VCC a 3,3 V |
| `LEVELS` | Durante 0,5 s cuenta cuántas muestras del GPIO26 están en alto |
| `CHECK` | `LEVELS` y después un pulso dominante de 3 ms. Comprueba que el transistor baja la línea y la suelta |
| `INIT HH` | Inicialización lenta a la dirección hexadecimal `HH` |
| `SCAN` | `INIT` de `1C`, `10`, `17` y `33`, con pausa entre ellas |
| `INIT 9D` | Rechazado. Responde `ADDR_9D_NO_ENVIADA` |

`SCAN` no envía servicios de lectura de memoria, no acciona salidas y no escribe Country, EKA ni EDC.

### Secuencia de cada INIT

1. Línea en reposo durante 2 s, para que un intento anterior termine.
2. Dirección a 5 baudios: bit de start en dominante, 8 bits de datos con el bit menos significativo primero, bit de stop en reposo. Cada bit dura 200 ms. El byte completo dura 2 s.
3. Espera como máximo 350 ms el flanco de bajada del byte de sincronismo.
4. Mide el ancho de ese primer bit bajo. Acepta entre 70 µs y 160 µs.
   - 9600 baudios ≈ 104 µs
   - 10400 baudios ≈ 96 µs
5. Lee el byte de sincronismo y dos bytes de clave (keywords) con ese ancho de bit.
6. Espera 30 ms y devuelve el complemento a uno del segundo keyword. Esa ventana, en ISO 9141, va de 25 ms a 50 ms. Sin este byte la ECU corta la sesión. No es una escritura de configuración.
7. Lee el byte siguiente. En una sesión ISO válida es el complemento de la dirección enviada.
8. Suelta la línea y publica el resultado por USB. No transmite nada más.

Entre una dirección y la siguiente de `SCAN` hay 3 s además de los 2 s de reposo del intento nuevo.

### Textos de resultado

| Texto | Significado |
| --- | --- |
| `KLINE_27VT_LISTO` | El sketch arrancó y la línea está en reposo |
| `RX_HIGH n/20` | Muestras en alto de `LEVELS` |
| `KLINE_EN_REPOSO` | 17 o más muestras en alto. La K-Line está en 12 V y el divisor la ve |
| `KLINE_EN_BAJO` | 3 o menos muestras en alto. Línea clavada a masa, transistor invertido o sin pull-up |
| `KLINE_INESTABLE` | Lectura a mitad de camino. Masa floja o contacto a medias |
| `PULSO OK` | El pulso de 3 ms se vio en el GPIO26 |
| `PULSO FALLO` | El transistor no mueve la línea que el divisor está leyendo |
| `SILENCIO` | Nadie respondió en 350 ms |
| `KLINE_QUEDA_EN_BAJO` | Apareció un flanco y la línea no volvió a reposo |
| `ANCHO_RARO` | Hubo un pulso fuera de 70–160 µs. Detrás va el ancho medido |
| `SYNC 55` | Byte de sincronismo ISO correcto |
| `RX_INVERTIDA` | El sincronismo llegó como `AA`, complemento de `55`. El divisor o el modo `IFACE` está al revés |
| `KW xx yy` | Dos keywords. Identifican el protocolo del módulo que contestó |
| `BAUD 9600` o `BAUD 10400` | Velocidad más cercana al ancho medido, con tolerancia de 250 baudios |
| `BITUS n` | Ancho crudo del primer bit, en microsegundos |
| `ECHO xx` | Byte que devolvió la ECU después del complemento del keyword |
| `ESPERADO xx` | Complemento de la dirección enviada. Tiene que coincidir con `ECHO` |
| `SESION_OK` | `SYNC 55` y el eco coincide con el complemento de la dirección |
| `SESION_PARCIAL` | Llegó el sincronismo y falta el eco, o el eco no coincide |
| `SESION_RARA` | Llegaron bytes y el primero no es `55` |
| `SCAN_FIN` | Terminaron las cuatro direcciones |

Ejemplo de sesión válida, con números ilustrativos:

```
INIT 1C
SYNC 55
KW 83 76
BAUD 10400
BITUS 96
ECHO E3
ESPERADO E3
SESION_OK
```

`E3` es el complemento de `1C`. Los keywords del ejemplo son los del Lucas 5AS en modo normal. Si la 27VT usa otros, el log los va a mostrar igual y esos son los que valen.

## Programa de registro

En la Raspberry Pi OS:

```bash
sudo apt install python3-serial
sudo usermod -a -G dialout $USER
python3 kline_log.py /dev/ttyUSB0
```

El grupo `dialout` aplica después de volver a entrar a la sesión. Si el puente USB del ESP32 aparece como otro dispositivo, se pasa por argumento: `python3 kline_log.py /dev/ttyACM0`.

Cada línea que imprime el ESP32 se copia a `host/logs/` con fecha y hora. Los comandos se escriben en esa misma terminal y salen hacia el ESP32. Ctrl-C cierra el puerto.

En la PC, el monitor serie del IDE de Arduino a 115200 hace el mismo trabajo. El sketch se flashea desde esa PC antes de salir al campo: la Pi Zero no es un entorno cómodo para compilar el core de ESP32.

## Procedimiento en la camioneta

Condiciones: contacto en posición II, motor parado, batería capaz de mantener el contacto, portón cerrado.

1. Armar el circuito. Conectar masa (pin 4) y K-Line (pin 7). Dejar el pin 16 libre.
2. Conectar el USB del ESP32 a la Pi o a la PC.
3. Abrir el registro o el monitor. Tiene que aparecer `KLINE_27VT_LISTO`.
4. `IFACE NPN` si el circuito es el transistor. `IFACE L9637` solo con el integrado a 3,3 V.
5. `LEVELS`. El resultado útil es `KLINE_EN_REPOSO`.
6. `CHECK`. El resultado útil es `PULSO OK`.
7. `SCAN`.
8. Guardar el archivo de `host/logs/`.

Orden de lectura del log:

- `PULSO FALLO` o `KLINE_EN_BAJO`: el cable o el transistor están mal. No tiene sentido interpretar las direcciones.
- Las cuatro direcciones en `SILENCIO`, con `PULSO OK` y `KLINE_EN_REPOSO`: la electricidad está bien y ninguna de esas direcciones despertó un módulo. El siguiente dato es probar otra dirección con `INIT HH`, de a una, nunca `9D`.
- `SESION_OK`: quedaron fijas la dirección, la velocidad y los keywords de un módulo de este vehículo. Recién con esos tres números se puede buscar el comando de lectura de Country en una captura compatible. La escritura sigue siendo un paso aparte, de un solo campo, sobre una lectura previa de esta misma CCU.

## Lo que este lector todavía no hace

No lee ni escribe el bloque de ajustes. Country, Transport mode, Tail window, VIN, EKA y EDC code siguen del lado de TestBook, T4, PScan o SM024 hasta que una sesión real entregue los comandos.

Cuando esa lectura exista, la escritura de reparación es una sola:

- Cambiar Country de 0 a 57.
- Si Transport mode está activo en la misma pantalla, apagarlo en esa misma edición.
- Copiar el resto de los campos tal como se leyeron en esa sesión, en especial EDC code, VIN `690228`, Engine = Diesel, Gearbox = Manual, Drive = LHD, Doors y Tail window = instalada.

El número EKA y el Master EKA son de solo lectura: se generan al fabricar la CCU. Las opciones escribibles "EKA" y "EKA delay" no forman parte de esta reparación. El EDC code es la clave con la que la CCU habilita el arranque del L-Series. Cargarlo desde otra camioneta, o modificarlo, deja el motor inmovilizado. Se sincroniza solo si se reemplaza la CCU o la ECU del motor.

Después de salir de Transit Mode el manual exige calibrar la luneta. Sin el dato de posición, el botón puede seguir sin completar la secuencia aunque Country ya sea 57.

1. Portón cerrado, vehículo abierto, alarma desactivada.
2. Desconectar la masa de la batería, esperar al menos 10 s y reconectar.
3. Unos 2 s después el vidrio baja solo hasta el tope.
4. Subirlo del todo, bajarlo del todo y subirlo otra vez hasta el tope.
5. Si queda arriba, la calibración cerró. Si la CCU pita cerca de 1 s y el vidrio baja solo, repetir desde el paso 1.

En el mapa de entradas del SM024, el botón del portón es **Door open request**. La luneta, al recibir esa entrada fuera de Transit Mode, pasa de `STATIONARY` a `GOTO COS`: baja lo justo para zafar el burlete y después suelta el pestillo. Durante la calibración los estados son `CAL DOWN`, `CAL UP` y `CAL STALL`.

## Referencias usadas

- Manual de taller Freelander, descripción de Transit Mode y calibración de la luneta.
- Blackbox Solutions, ayuda SM024 v1.22, campos Country, Transport mode y entradas de la 27VT.
- Conector de diagnóstico C0040: pin 7 = ISO 9141 K-Line, pin 4 = masa, pin 16 = batería.
- Investigación local del repositorio: `lucas_27vt_investigacion.md`, `Proyecto_Roverta_Lucas27VT.md`.
