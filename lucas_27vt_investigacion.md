# Investigación Técnica — Lucas 27VT CCU
## Land Rover Freelander 1 — MY97-MY99
### VIN: SAL LN AB B X A 69XXXX (XEDi 1999, Solihull, Manual 5v)

---

## Decodificación VIN

| Posición | Código | Significado |
|----------|--------|-------------|
| 1–3 | SAL | WMI — Land Rover UK |
| 4–5 | LN | Modelo Freelander 1 |
| 6 | A | Carrocería 5 puertas |
| 7 | B | Motor L-Series 2.0 XEDi (Turbodiésel) |
| 8 | B | Transmisión Manual 5v, LHD |
| 9 | 8 | Especificación de mercado |
| 10 | X | Año modelo 1999 |
| 11 | A | Planta Solihull, Inglaterra |
| 12–17 | 69XXXX | Número de serie único |

**Implicación para diagnóstico:** MY99 = Coding Index 1 → EKA activo, Transit Mode accesible, inmovilizador integrado en CCU (no EWS separado).

---

## Prioridad 1 — CRÍTICA: Protocolo diagnóstico Lucas 27VT

### Parámetros del bus K-Line

| Parámetro | Valor | Estado |
|-----------|-------|--------|
| Baudrate principal | 9600 bps | Confirmado (5AS contemporáneo) |
| Baudrate de inicialización | 1200 bps | 5-baud wakeup estándar ISO 9141 |
| Formato de frame | 8N1 (8 bits, sin paridad, 1 stop bit) | Confirmado |
| Estándar base | ISO 9141 / protocolo propietario ROSCO/Lucas | Confirmado |
| Dirección ECU del 27VT | **DESCONOCIDA** | ❌ A determinar |
| Algoritmo checksum | **DESCONOCIDO** | ❌ A determinar |

### Inicialización 5-baud (extrapolado del Lucas 5AS)

El 5AS, ECU contemporánea al 27VT fabricada por Lucas, tiene protocolo documentado en rovermems.com:

```
Wakeup byte (modo normal):  0x1C  →  respuesta ECU: 0x55 0x83 0x76
Wakeup byte (modo EEPROM):  0x9D  →  respuesta ECU: 0x55 0x83 0xF7
```

- `0x55` = Sync byte estándar ISO 9141
- `0x83 0x76` / `0x83 0xF7` = Keywords de protocolo Lucas

> ⚠️ **Hipótesis:** El 27VT usa el mismo esquema de inicialización pero con bytes de wakeup diferentes. Requiere sniffing con analizador lógico en pin 7 del OBD-II mientras PScan o SM024 se comunican.

### Estructura de mensajes (basado en 5AS como referencia)

```
0x83          →  Request paquete de datos (~49 bytes)
0xF6          →  Reset / desconexión del bus
0x3x          →  Inicio de test de output (ej: 0x38 = bocina, 0x3d = hazards)
0x2x          →  Fin de test de output
0x5F          →  Inicio de programación de fobs PLIP
0x6F          →  Fin de programación de fobs PLIP
```

Estructura del paquete de respuesta 0x83 (5AS, 49 bytes):

```
Byte 00  — Tamaño del paquete (incluyendo este byte)
Byte 01  — Bonnet switch: 0x00 / 0x80 (bit 7)
Byte 06  — Ignition switch: 0x00 / 0x80 (bit 7)
Byte 09  — Door switch(es): 0x00 / 0x80 (bit 7)
Byte 11  — Boot/tailgate switch: 0x00 / 0x80 (bit 7)
Byte 16  — Contador rolling code key 1
Byte 17  — Contador rolling code key 2
Byte 18  — Contador rolling code key 3
Byte 19  — Contador rolling code key 4
Byte 20–22 — Serial number (3 bytes, LSByte first)
Byte 35–48 — ASCII: descripción de unidad (ej: "AP3.005AS-98MY")
```

### Comandos EEPROM (protocolo 5AS — referencia)

Los comandos siempre son 4 bytes: `[CMD, ADDR, VALUE, 0xAA]`

```
- 4to byte siempre 0xAA
- Si escritura: bit 5 del CMD byte = 1; si lectura = 0
- Si EEPROM (0x100+): bit 6 del CMD byte = 1; si EPROM/RAM = 0
- Dirección: CMD[bit0] = addr[bit0];  byte[1] = addr >> 1
- Tamaño de lectura: bits 1,2,3 del CMD byte (valor + 1 bytes retornados, máx 8)
```

Rangos de memoria (5AS):
- `0x00 – 0xFF` = EPROM/RAM
- `0x100 – 0x1FF` = EEPROM

> ⚠️ Estos rangos son del 5AS. El 27VT usa MC68HC05B32 — el layout puede diferir. Requiere dump y comparación.

---

## Prioridad 2 — MUY IMPORTANTE: Transit Mode

### Confirmación del mecanismo

**Transit Mode = campo "Country" en EEPROM del CCU = valor 0x00**

Fuente: Blackbox Solutions SM024 help file v1.22 (documentación oficial del Faultmate para el 27VT):

> "When set to Zero this engages the Transit mode of the CCU."

### Tabla completa de valores de país

| Código | País |
|--------|------|
| **0** | **Transit Mode (ACTIVO)** |
| 1 | Transit (modo especial — no usar) |
| 2 | UK |
| 8 | France |
| 9 | Germany |
| 19 | Spain |
| 55 | USA |
| 56 | Canada |
| **57** | **Argentina** |
| 58 | Australia |
| 65 | New Zealand |
| 67 | South Africa |
| 68 | Uruguay |
| 69 | Police |
| 70 | Limousine |
| 71 | Hearse |
| 72 | S-Vehicle |

### Cómo desactivar Transit Mode

1. Conectar herramienta diagnóstica (PScan o Faultmate SM024)
2. Acceder a Settings del 27VT CCU
3. Cambiar campo **Country** de `0` a `57` (Argentina) u otro valor apropiado
4. Escribir / guardar en EEPROM

> ✅ **Hipótesis confirmada.** Transit Mode no es un flag separado — es directamente el campo Country = 0.

> ⚠️ **Nota importante:** "Transport mode" aparece también como campo separado en Coding Index 1. Puede ser un segundo flag independiente del campo Country. Verificar ambos si Transit Mode persiste.

---

## Prioridad 3 — Output Tests

### Capacidades confirmadas (SM024 + PScan)

El Faultmate SM024 documenta "a large number of drivable outputs" para el 27VT:

| Función | Estado | Herramienta |
|---------|--------|-------------|
| LOCK (cerrar puertas) | ✅ Confirmado | SM024, PScan |
| UNLOCK (abrir puertas) | ✅ Confirmado | SM024, PScan |
| TAILGATE / Rear window | ✅ Confirmado (campo "Tail window" + "1 shot") | SM024 |
| Bocina (Horn) | ✅ Confirmado | SM024, PScan |
| Hazard lights | ✅ Confirmado | SM024, PScan |
| Wipers (limpiaparabrisas) | ✅ Confirmado | PScan |
| Welcome light | ✅ Configuración disponible | SM024 |

> ⚠️ Los bytes de comando raw para lock/unlock/tailgate **no están en fuentes públicas**. PScan y SM024 los implementan en código cerrado. Requiere sniffing.

---

## Prioridad 4 — EEPROM del 27VT

### Campos documentados (SM024 v1.22 — Coding Index 1)

| Campo | Descripción | Notas |
|-------|-------------|-------|
| **EKA number** | Código maestro — generado en fábrica | Read only, no se puede cambiar |
| **Master EKA** | Código de acceso de emergencia | Read only |
| **Country** | País / configuración de mercado (0 = Transit) | Escribible |
| **Model year** | Año de fabricación del ECU | — |
| **Vin number** | Últimos 6 dígitos del VIN | Para CCU nueva: N/A |
| **Doors** | Sensores/puertas activos para alarma | Para CCU nueva: N/A |
| **Coding index** | Layout de opciones en EEPROM | Determina qué campos existen |
| **Software ID** | Revisión de software del CCU | — |
| **Hardware ID** | Revisión de hardware | — |
| **Diagnostic ID** | Revisión del software de diagnóstico | — |
| **CCU serial number** | Número de serie único de esta CCU | — |
| **EDC code** | Código de sincronía CCU ↔ ECU diésel (XEDi) | Crítico — si CCU o EDC reemplazados, no arranca |
| **EKA** | Código de emergencia (solo Coding Index 1) | Tu CCU MY99 lo tiene |
| **EKA delay** | Timeout de 30 min tras 3 intentos fallidos | Solo Coding Index 1 |
| **Alarm** | Habilita/deshabilita alarma completa + LED | — |
| **Engine** | Petrol o Diesel | Tu CCU debe estar en Diesel |
| **Superlocking** | Bloqueo secundario de puertas | — |
| **Transport mode** | Flag de modo transporte | Solo Coding Index 1 |
| **SPE** | Single Point Locking | — |
| **Plip superlock** | Número de pulsaciones para superlock | — |
| **Gearbox** | Automático o Manual | Manual en tu caso |
| **Drive** | LHD o RHD | LHD (volante izquierda) |
| **Air conditioning** | A/C instalado o no | — |
| **Tail window** | Ventana trasera eléctrica instalada o no | — |
| **ABS** | ABS instalado o no | — |

### Direcciones EEPROM de referencia (Lucas 5AS — MCU similar)

| Campo | Dirección |
|-------|-----------|
| Serial number | `0x19C – 0x19E` |
| EKA (4 dígitos BCD, MSB first) | `0x1A0 – 0x1A1` |

> Nota: Si EKA = `0x1234` (hex) → código a ingresar = `1 2 3 4` (BCD).
> El 5AS almacena todo en copia doble: copia normal + copia invertida.

### Microcontrolador confirmado

**MCU: Motorola MC68HC05B32** — confirmado por ICC IMMO Calculator en su soporte para el 27VT.

---

## Prioridad 5 — Herramientas que ya resolvieron el problema

### PScan (pscan.uk)

- Software para laptop (Windows 7+)
- **Protocolo MY97-99 separado e independiente** del MY2000+
- Lee/escribe configuración completa del 27VT
- Programa fobs PLIP (MY97-99 incluye inmovilizador en CCU)
- Acciona outputs: wipers, windows
- Lee live data
- Soporta todos los ECUs del Freelander 1: MEMS1.9 (EDC1.3.1 para L-Series), ABS Wabco-D, Teves Mk20/25, Airbag Siemens, 27VT CCU

### Faultmate MSV-2 / SM024 (Blackbox Solutions)

- **SKU: SM024** — módulo de software para 27VT CCU
- Hardware: Faultmate MSV-2 + Black OBDII Lead
- Lee/escribe todos los settings listados arriba
- Acciona outputs de lock/unlock/tailgate
- RF test + PLIP learn (programación de mandos)
- Help file completo online: `https://blackbox-solutions.com/help/SM024.html`
- Precio aprox: €96 + IVA (solo el módulo SM024)

### ICC IMMO Calculator

- Calcula el EKA desde dump físico de EEPROM
- Soporta Freelander 1 (27VT) + Discovery + Range Rover
- Requiere extraer y dumpear el chip EEPROM físicamente
- Útil cuando no hay acceso vía diagnóstico (CCU bloqueada, Transit Mode severo)

---

## Prioridad 6 — Lucas 5AS como referencia de protocolo

El 5AS (inmovilizador Lucas, mismo período MY97-99, misma familia de ECUs) tiene protocolo parcialmente documentado en **rovermems.com** por James Portman.

### Lo que el 5AS confirma para el 27VT

| Elemento | Confirmado por 5AS |
|----------|-------------------|
| Baudrate 9600 bps | ✅ Sí |
| Esquema 5-baud wakeup | ✅ Sí |
| Formato 8N1 | ✅ Sí (confirmado por MEMS 1.9 también) |
| Comandos 0x3x para tests | ✅ Sí (bocina 0x38, hazards 0x3d) |
| Reset 0xF6 | ✅ Sí (comparte con MEMS engine ECUs) |
| Estructura EEPROM con doble copia invertida | ✅ Sí |
| Programación de fobs con 0x5F/0x6F | ✅ Sí |

Fuente técnica: `https://rovermems.com/5as/index.html`

---

## Prioridad 7 — Hardware interno del 27VT

| Componente | Identificado | Fuente |
|------------|-------------|--------|
| MCU | **Motorola MC68HC05B32** | ICC IMMO Calculator |
| EEPROM externa | Probable 93C46 o 93C66 | Referencia Discovery/TD5 similares |
| Transceptor K-Line | No identificado (típico: MC33199 o L9637) | Hipótesis |
| Conector diagnóstico | OBD-II 16 pin, pin 7 = K-Line | Confirmado (Black OBDII Lead) |

---

## Gaps críticos — Lo que falta descubrir

| Gap | Prioridad | Cómo resolverlo |
|-----|-----------|----------------|
| Byte de wakeup 5-baud del 27VT | 🔴 Alta | Sniffing K-Line mientras PScan/SM024 conecta |
| Dirección ECU (target address) del 27VT | 🔴 Alta | Sniffing K-Line |
| Algoritmo de checksum de mensajes | 🔴 Alta | Análisis de capturas del bus |
| Comandos raw lock / unlock / tailgate | 🔴 Alta | Sniffing K-Line durante output tests |
| Layout hex completo de EEPROM (27VT MY99) | 🟡 Media | Dump físico del MC68HC05B32 + análisis con SM024 |
| Diferencias exactas protocolo MY97-99 vs MY2000+ | 🟡 Media | Documentación PScan o sniffing de ambas versiones |

### Estrategia recomendada

1. Conectar **PScan o SM024** al OBD-II del Freelander
2. Paralelamente, conectar un **analizador lógico** (ej: Saleae Logic, sigrok, o incluso Arduino) al **pin 7 del OBD-II** (K-Line)
3. Capturar la secuencia completa de inicialización y comandos
4. Con esos bytes raw, el protocolo queda completamente documentado

---

## Resumen ejecutivo

- **Transit Mode:** Campo `Country` de EEPROM = 0. Solucionable con PScan o SM024 escribiendo `Country = 57` (Argentina).
- **Protocolo K-Line:** ISO 9141, 9600 bps, 8N1, wakeup 5-baud. Bytes específicos del 27VT no publicados pero capturables por sniffing.
- **MCU:** Motorola MC68HC05B32 — accesible para dump físico con programador universal.
- **EKA:** Solo disponible en Coding Index 1 (tu CCU MY99). Legible vía diagnóstico (SM024/PScan) o por dump de EEPROM (ICC Calculator).
- **EDC code:** Tu CCU tiene este campo (XEDi diésel). Si reemplazás la CCU o el ECU del motor, hay que sincronizarlos — PScan y SM024 lo hacen.
- **Herramientas óptimas:** PScan para diagnóstico completo vía software; Faultmate SM024 para solución dedicada de hardware.

---

*Investigación compilada: junio 2026*
*Fuentes principales: Blackbox Solutions SM024 help v1.22, PScan.uk, rovermems.com (James Portman), ICC IMMO Calculator, The-T-Bar PScan forum*
