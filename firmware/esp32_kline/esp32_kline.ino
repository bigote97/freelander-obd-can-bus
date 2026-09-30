/*
  Lector K-Line para Freelander 1 MY99 — Lucas 27VT
  Placa: ESP32. La Raspberry Pi Zero W solo registra el puerto serie.

  La CCU no está en CAN. El pin 7 del conector OBD es K-Line (12 V).
  El pin 4 es masa. El pin 16 es batería: no conectarlo a ningún pin del ESP32.

  Este programa solo despierta módulos con la inicialización lenta ISO 9141
  y muestra los bytes de respuesta. No envía escrituras de configuración.

  Circuito con un NPN (2N2222, BC547 o similar), mirando el zócalo del auto:

      pin 4  ---------------- GND del ESP32
      pin 7  ----+---- 150k ---- GPIO26 (RX)
                 |                 |
                 |                3V3 zener (cátodo en GPIO26, ánodo a GND)
                 |                 |
                 |                47k
                 |                 |
                 |                GND
                 |
                 +---- 1k ---- colector NPN
      GPIO25 ---- 1k ---- base NPN
      base ---- 100k ---- GND
      emisor NPN ---------- GND

  Sin el transistor, el pin 7 no se conecta. 12 V quema el ESP32.
  En el monitor serie, antes de SCAN, ejecutar: IFACE NPN
  Si más adelante hay un L9637 alimentado a 3,3 V (no a 5 V): IFACE L9637

  Contacto en posición II, motor parado. Flashear a 115200 baud.
*/

static const int PIN_TX = 25;
static const int PIN_RX = 26;
static const uint32_t USB_BAUD = 115200;

static const uint8_t SCAN_ADDRS[] = {0x1C, 0x10, 0x17, 0x33};
static const int SCAN_COUNT = sizeof(SCAN_ADDRS);

static bool ifaceNpn = true;

static void setRecessive() {
  pinMode(PIN_TX, OUTPUT);
  digitalWrite(PIN_TX, ifaceNpn ? LOW : HIGH);
}

static void setDominant() {
  pinMode(PIN_TX, OUTPUT);
  digitalWrite(PIN_TX, ifaceNpn ? HIGH : LOW);
}

static void setLogic(bool high) {
  if (high) setRecessive();
  else setDominant();
}

static void waitUs(uint32_t us) {
  uint32_t start = micros();
  while ((uint32_t)(micros() - start) < us) {
  }
}

static bool waitLevel(int level, uint32_t timeoutMs) {
  uint32_t start = millis();
  while (digitalRead(PIN_RX) != level) {
    if ((uint32_t)(millis() - start) >= timeoutMs) return false;
  }
  return true;
}

static void sendSlowAddress(uint8_t addr) {
  setLogic(false);
  delay(200);
  for (int i = 0; i < 8; i++) {
    setLogic((addr >> i) & 0x01);
    delay(200);
  }
  setLogic(true);
  delay(200);
}

static bool readByte(uint32_t edgeUs, uint32_t bitUs, uint8_t &value) {
  uint8_t v = 0;
  for (int i = 0; i < 8; i++) {
    uint32_t target = edgeUs + (bitUs * 3) / 2 + bitUs * i;
    while ((int32_t)(target - micros()) > 0) {
    }
    if (digitalRead(PIN_RX)) v |= (1 << i);
  }
  uint32_t stopAt = edgeUs + (bitUs * 3) / 2 + bitUs * 8;
  while ((int32_t)(stopAt - micros()) > 0) {
  }
  value = v;
  return true;
}

static bool readFramedByte(uint32_t bitUs, uint32_t timeoutMs, uint8_t &value) {
  if (!waitLevel(LOW, timeoutMs)) return false;
  uint32_t edge = micros();
  return readByte(edge, bitUs, value);
}

static void sendFastByte(uint32_t bitUs, uint8_t value) {
  uint32_t edge = micros();
  setLogic(false);
  for (int i = 0; i < 8; i++) {
    uint32_t target = edge + bitUs * (i + 1);
    while ((int32_t)(target - micros()) > 0) {
    }
    setLogic((value >> i) & 0x01);
  }
  uint32_t stopAt = edge + bitUs * 9;
  while ((int32_t)(stopAt - micros()) > 0) {
  }
  setLogic(true);
  waitUs(bitUs);
}

static uint32_t nearestBaud(uint32_t bitUs) {
  uint32_t baud = 1000000UL / bitUs;
  uint32_t d9600 = baud > 9600 ? baud - 9600 : 9600 - baud;
  uint32_t d10400 = baud > 10400 ? baud - 10400 : 10400 - baud;
  if (d9600 <= 250) return 9600;
  if (d10400 <= 250) return 10400;
  return baud;
}

static bool initAddress(uint8_t addr) {
  Serial.print("INIT ");
  Serial.println(addr, HEX);
  setRecessive();
  delay(2000);
  sendSlowAddress(addr);

  uint32_t bitUs = 0;
  uint32_t edge = 0;
  if (!waitLevel(LOW, 350)) {
    Serial.println("SILENCIO");
    setRecessive();
    return false;
  }
  edge = micros();
  if (!waitLevel(HIGH, 20)) {
    Serial.println("KLINE_QUEDA_EN_BAJO");
    setRecessive();
    return false;
  }
  bitUs = (uint32_t)(micros() - edge);
  if (bitUs < 70 || bitUs > 160) {
    Serial.print("ANCHO_RARO ");
    Serial.println(bitUs);
    setRecessive();
    return false;
  }

  uint8_t sync = 0;
  uint8_t kw1 = 0;
  uint8_t kw2 = 0;
  uint8_t echo = 0;
  bool syncOk = readByte(edge, bitUs, sync);
  bool kw1Ok = syncOk && readFramedByte(bitUs, 40, kw1);
  bool kw2Ok = kw1Ok && readFramedByte(bitUs, 40, kw2);

  bool echoOk = false;
  if (kw2Ok) {
    delay(30);
    sendFastByte(bitUs, (uint8_t)~kw2);
    echoOk = readFramedByte(bitUs, 80, echo);
  }
  setRecessive();

  Serial.print("SYNC ");
  if (syncOk) Serial.println(sync, HEX);
  else Serial.println("NA");
  Serial.print("KW ");
  if (kw1Ok) Serial.print(kw1, HEX);
  else Serial.print("NA");
  Serial.print(" ");
  if (kw2Ok) Serial.println(kw2, HEX);
  else Serial.println("NA");
  Serial.print("BAUD ");
  Serial.println(nearestBaud(bitUs));
  Serial.print("BITUS ");
  Serial.println(bitUs);
  Serial.print("ECHO ");
  if (echoOk) Serial.println(echo, HEX);
  else Serial.println("NA");
  uint8_t expect = (uint8_t)~addr;
  Serial.print("ESPERADO ");
  Serial.println(expect, HEX);
  if (syncOk && sync == 0xAA) Serial.println("RX_INVERTIDA");
  if (syncOk && sync == 0x55 && echoOk && echo == expect) {
    Serial.println("SESION_OK");
  } else if (syncOk) {
    Serial.println("SESION_PARCIAL");
  } else {
    Serial.println("SESION_RARA");
  }
  return syncOk;
}

static void printLevels() {
  setRecessive();
  delay(50);
  int highs = 0;
  for (int i = 0; i < 20; i++) {
    if (digitalRead(PIN_RX)) highs++;
    delay(25);
  }
  Serial.print("RX_HIGH ");
  Serial.print(highs);
  Serial.println("/20");
  if (highs > 16) Serial.println("KLINE_EN_REPOSO");
  else if (highs < 4) Serial.println("KLINE_EN_BAJO");
  else Serial.println("KLINE_INESTABLE");
}

static void checkLoopback() {
  printLevels();
  setDominant();
  delay(3);
  int low = digitalRead(PIN_RX) == LOW;
  setRecessive();
  delay(3);
  int high = digitalRead(PIN_RX) == HIGH;
  Serial.print("PULSO ");
  Serial.println(low && high ? "OK" : "FALLO");
  if (!(low && high)) {
    Serial.println("Revisar transistor, masa pin 4 y divisor del pin 7.");
  }
}

static void printHelp() {
  Serial.println("IFACE NPN|L9637");
  Serial.println("LEVELS");
  Serial.println("CHECK");
  Serial.println("INIT HH");
  Serial.println("SCAN");
  Serial.println("Solo lectura. Contacto en II, motor parado.");
}

static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static void handleLine(String line) {
  line.trim();
  if (line.length() == 0) return;
  line.toUpperCase();
  if (line == "HELP" || line == "?") {
    printHelp();
  } else if (line == "IFACE NPN") {
    ifaceNpn = true;
    setRecessive();
    Serial.println("IFACE NPN");
  } else if (line == "IFACE L9637") {
    ifaceNpn = false;
    setRecessive();
    Serial.println("IFACE L9637");
  } else if (line == "LEVELS") {
    printLevels();
  } else if (line == "CHECK") {
    checkLoopback();
  } else if (line == "SCAN") {
    for (int i = 0; i < SCAN_COUNT; i++) {
      initAddress(SCAN_ADDRS[i]);
      delay(3000);
    }
    Serial.println("SCAN_FIN");
  } else if (line.startsWith("INIT ")) {
    if (line.length() < 7) {
      Serial.println("USO INIT HH");
      return;
    }
    int hi = hexNibble(line.charAt(5));
    int lo = hexNibble(line.charAt(6));
    if (hi < 0 || lo < 0) {
      Serial.println("USO INIT HH");
      return;
    }
    uint8_t addr = (uint8_t)((hi << 4) | lo);
    if (addr == 0x9D) {
      Serial.println("ADDR_9D_NO_ENVIADA");
      return;
    }
    initAddress(addr);
  } else {
    Serial.println("NO_ENTIENDO");
    printHelp();
  }
}

void setup() {
  pinMode(PIN_RX, INPUT);
  setRecessive();
  Serial.begin(USB_BAUD);
  delay(300);
  Serial.println("KLINE_27VT_LISTO");
  Serial.println("IFACE NPN");
  printHelp();
}

void loop() {
  static String buf;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      handleLine(buf);
      buf = "";
    } else if (buf.length() < 32) {
      buf += c;
    }
  }
}
