/**********************************************************
* Ventilador Controlado por 1 push button                 *
* Versao 5.0/2026 - Desenvolvido por JulioLobo            *
*                                                         *
* Esse projeto pode ser usado em ventiladores que tenham  *
* uma chave seletora de velocidade com 3 velocidades      *
* Nao e compativel com ventiladores com DIMMER            *
*                                                         *
* Troque a chave seletora de velocidade do ventilador por *
* um push button.                                         *
* Usei um NodeMCU com um modulo de 4 reles.               *
* Os 3 pinos que iriam para a chave usei 1 em cada rele.  *
* O esquema de ligacao esta junto deste codigo            *
*                                                         *
* Bibliotecas para instalar no Arduino IDE:               *
*   - WiFiManager by tzapu/tablatronix                    *
*   - ArduinoJson by Benoit Blanchon                      *
*   - ESP8266 core com LittleFS e mDNS                    *
*                                                         *
* Voce e livre para modificar e usar esse codigo, so peco *
* que mantenha os creditos, obrigado por respeitar isso!  *
***********************************************************/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

/* -----------------------------------------
| NodeMCU / ESP8266  |  NodeMCU / ESP8266  |
| D0 = 16            |  D6 = 12            |
| D1 = 5             |  D7 = 13            |
| D2 = 4             |  D8 = 15            |
| D3 = 0             |  D9 = 3             |
| D4 = 2             |  D10 = 1            |
| D5 = 14            |                     |
------------------------------------------*/

const int btnPin = 14;
const int conmin = 12;
const int conmed = 5;
const int conmax = 4;
const unsigned long tempoTrocaRele = 180;

const char* AP_SSID = "Ventilador-Config";
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 20000;
const char* CONFIG_FILE = "/config.json";

struct AppConfig {
  char host[33];
  char ip[16];
  char gw[16];
  char sn[16];
  bool forcePortalOnBoot;
};

AppConfig appConfig = { "ventilador", "", "", "", false };

int velocidade = 0;
unsigned long lastButtonPress = 0;
unsigned long wifiConnectStartedAt = 0;
unsigned long restartAt = 0;

bool fsReady = false;
bool portalActive = false;
bool networkReady = false;
bool mainServerStarted = false;
bool mdnsStarted = false;
bool shouldSaveConfig = false;
bool restartScheduled = false;
bool reconnectAttemptInProgress = false;

ESP8266WebServer server(80);
WiFiManager wm;

char hostBuffer[33] = "ventilador";
char ipBuffer[16] = "";
char gwBuffer[16] = "";
char snBuffer[16] = "";

WiFiManagerParameter hostHelp(
  "<p style='margin:8px 0 12px'><strong>Acesso local:</strong> http://nomedohost.local, caso deixe em branco será http://ventilador.local</p>"
);
WiFiManagerParameter hostParam("host", "Nome do Host (sem .local)", hostBuffer, sizeof(hostBuffer) - 1);
WiFiManagerParameter ipParam("ip", "IP local (vazio = DHCP)", ipBuffer, sizeof(ipBuffer) - 1);
WiFiManagerParameter gwParam("gw", "Gateway", gwBuffer, sizeof(gwBuffer) - 1);
WiFiManagerParameter snParam("sn", "Subnet", snBuffer, sizeof(snBuffer) - 1);

void desligarReles();
void definesaida(int saida);
void handleRoot();
void handleWifiPage();
void handleWifiStart();
void handleFaviconSvg();
void handleNotFound();
void renderHomePage();
void renderWifiPage(const String& message, bool showStartButton);
void sendHtmlStart();
void sendHtmlEnd();
void sendContent(const String& content);
void processButton();
void processWiFiState();
void beginStationConnection();
void beginConfigPortal(const char* reason);
void onConnectedToNetwork();
void onDisconnectedFromNetwork();
void configureRoutes();
void configureWiFiManager();
bool mountFileSystem();
void loadConfig();
void saveConfig();
void copyParametersFromPortal();
void saveConfigCallback();
void scheduleRestart(unsigned long delayMs);
void sanitizeHostname(const char* input, char* output, size_t outputLen);
void copyStringSafe(char* dest, size_t destLen, const char* src);
bool parseIpAddress(const char* value, IPAddress& result);
bool hasStaticNetworkConfig(IPAddress& ip, IPAddress& gw, IPAddress& sn);
void applySavedNetworkPreferences();
String currentHostUrl();

void saveConfigCallback() {
  shouldSaveConfig = true;
}

bool mountFileSystem() {
  if (fsReady) {
    return true;
  }

  fsReady = LittleFS.begin();
  if (!fsReady) {
    Serial.println("[FS] Falha ao montar LittleFS.");
  }
  return fsReady;
}

void copyStringSafe(char* dest, size_t destLen, const char* src) {
  if (destLen == 0) {
    return;
  }

  if (src == nullptr) {
    dest[0] = '\0';
    return;
  }

  strlcpy(dest, src, destLen);
}

void sanitizeHostname(const char* input, char* output, size_t outputLen) {
  const char* fallback = "ventilador";
  size_t writeIndex = 0;

  if (outputLen == 0) {
    return;
  }

  output[0] = '\0';

  if (input == nullptr) {
    strlcpy(output, fallback, outputLen);
    return;
  }

  for (size_t i = 0; input[i] != '\0' && writeIndex + 1 < outputLen; ++i) {
    char c = input[i];

    if (c >= 'A' && c <= 'Z') {
      c = static_cast<char>(c - 'A' + 'a');
    }

    bool isLetter = (c >= 'a' && c <= 'z');
    bool isDigit = (c >= '0' && c <= '9');
    bool isHyphen = (c == '-');

    if (!(isLetter || isDigit || isHyphen)) {
      continue;
    }

    if (writeIndex == 0 && isHyphen) {
      continue;
    }

    output[writeIndex++] = c;
  }

  while (writeIndex > 0 && output[writeIndex - 1] == '-') {
    --writeIndex;
  }

  output[writeIndex] = '\0';

  if (writeIndex == 0) {
    strlcpy(output, fallback, outputLen);
  }
}

bool parseIpAddress(const char* value, IPAddress& result) {
  if (value == nullptr || strlen(value) < 7) {
    return false;
  }

  return result.fromString(value);
}

bool hasStaticNetworkConfig(IPAddress& ip, IPAddress& gw, IPAddress& sn) {
  return parseIpAddress(appConfig.ip, ip) &&
         parseIpAddress(appConfig.gw, gw) &&
         parseIpAddress(appConfig.sn, sn);
}

void loadConfig() {
  if (!mountFileSystem()) {
    return;
  }

  if (!LittleFS.exists(CONFIG_FILE)) {
    return;
  }

  File file = LittleFS.open(CONFIG_FILE, "r");
  if (!file) {
    Serial.println("[Config] Nao foi possivel abrir config.json.");
    return;
  }

  StaticJsonDocument<384> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println("[Config] JSON invalido, usando padrao.");
    return;
  }

  sanitizeHostname(doc["host"] | "ventilador", appConfig.host, sizeof(appConfig.host));
  copyStringSafe(appConfig.ip, sizeof(appConfig.ip), doc["ip"] | "");
  copyStringSafe(appConfig.gw, sizeof(appConfig.gw), doc["gw"] | "");
  copyStringSafe(appConfig.sn, sizeof(appConfig.sn), doc["sn"] | "");
  appConfig.forcePortalOnBoot = doc["forcePortalOnBoot"] | false;

  copyStringSafe(hostBuffer, sizeof(hostBuffer), appConfig.host);
  copyStringSafe(ipBuffer, sizeof(ipBuffer), appConfig.ip);
  copyStringSafe(gwBuffer, sizeof(gwBuffer), appConfig.gw);
  copyStringSafe(snBuffer, sizeof(snBuffer), appConfig.sn);
}

void saveConfig() {
  if (!mountFileSystem()) {
    return;
  }

  StaticJsonDocument<384> doc;
  doc["host"] = appConfig.host;
  doc["ip"] = appConfig.ip;
  doc["gw"] = appConfig.gw;
  doc["sn"] = appConfig.sn;
  doc["forcePortalOnBoot"] = appConfig.forcePortalOnBoot;

  File file = LittleFS.open(CONFIG_FILE, "w");
  if (!file) {
    Serial.println("[Config] Nao foi possivel salvar config.json.");
    return;
  }

  serializeJson(doc, file);
  file.close();
  Serial.println("[Config] Configuracao salva.");
}

void copyParametersFromPortal() {
  char sanitizedHost[sizeof(appConfig.host)];

  sanitizeHostname(hostParam.getValue(), sanitizedHost, sizeof(sanitizedHost));
  copyStringSafe(appConfig.host, sizeof(appConfig.host), sanitizedHost);
  copyStringSafe(appConfig.ip, sizeof(appConfig.ip), ipParam.getValue());
  copyStringSafe(appConfig.gw, sizeof(appConfig.gw), gwParam.getValue());
  copyStringSafe(appConfig.sn, sizeof(appConfig.sn), snParam.getValue());

  copyStringSafe(hostBuffer, sizeof(hostBuffer), appConfig.host);
  copyStringSafe(ipBuffer, sizeof(ipBuffer), appConfig.ip);
  copyStringSafe(gwBuffer, sizeof(gwBuffer), appConfig.gw);
  copyStringSafe(snBuffer, sizeof(snBuffer), appConfig.sn);
}

void applySavedNetworkPreferences() {
  IPAddress ip;
  IPAddress gw;
  IPAddress sn;

  WiFi.mode(WIFI_STA);
  WiFi.hostname(appConfig.host);

  if (hasStaticNetworkConfig(ip, gw, sn)) {
    WiFi.config(ip, gw, sn);
    Serial.print("[WiFi] IP fixo configurado: ");
    Serial.println(ip);
  } else {
    WiFi.config(0U, 0U, 0U);
    Serial.println("[WiFi] Usando DHCP.");
  }
}

void configureWiFiManager() {
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setBreakAfterConfig(true);
  wm.setConfigPortalBlocking(false);
  wm.setConnectTimeout(20);
  wm.setAPClientCheck(true);
  wm.addParameter(&hostHelp);
  wm.addParameter(&hostParam);
  wm.addParameter(&ipParam);
  wm.addParameter(&gwParam);
  wm.addParameter(&snParam);
}

void configureRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/wifi", HTTP_GET, handleWifiPage);
  server.on("/wifi/start", HTTP_GET, handleWifiStart);
  server.on("/favicon.svg", HTTP_GET, handleFaviconSvg);
  server.on("/favicon.ico", HTTP_GET, handleFaviconSvg);
  server.onNotFound(handleNotFound);
}

String currentHostUrl() {
  return String("http://") + appConfig.host + ".local";
}

void scheduleRestart(unsigned long delayMs) {
  restartScheduled = true;
  restartAt = millis() + delayMs;
}

void desligarReles() {
  digitalWrite(conmax, LOW);
  digitalWrite(conmed, LOW);
  digitalWrite(conmin, LOW);
}

void definesaida(int saida) {
  if (saida < 0 || saida > 3) {
    return;
  }

  if (saida == velocidade) {
    return;
  }

  desligarReles();

  if (saida != 0) {
    delay(tempoTrocaRele);
  }

  switch (saida) {
    case 0:
      velocidade = 0;
      break;
    case 1:
      digitalWrite(conmin, HIGH);
      velocidade = 1;
      break;
    case 2:
      digitalWrite(conmed, HIGH);
      velocidade = 2;
      break;
    case 3:
      digitalWrite(conmax, HIGH);
      velocidade = 3;
      break;
  }
}

void beginStationConnection() {
  reconnectAttemptInProgress = true;
  wifiConnectStartedAt = millis();
  applySavedNetworkPreferences();
  WiFi.begin();
  Serial.println("[WiFi] Tentando conectar com credenciais salvas...");
}

void beginConfigPortal(const char* reason) {
  if (portalActive) {
    return;
  }

  Serial.print("[WiFi] Iniciando portal: ");
  Serial.println(reason);

  if (mainServerStarted) {
    server.stop();
    mainServerStarted = false;
  }

  if (mdnsStarted) {
    MDNS.close();
    mdnsStarted = false;
  }

  networkReady = false;
  reconnectAttemptInProgress = false;

  copyStringSafe(hostBuffer, sizeof(hostBuffer), appConfig.host);
  copyStringSafe(ipBuffer, sizeof(ipBuffer), appConfig.ip);
  copyStringSafe(gwBuffer, sizeof(gwBuffer), appConfig.gw);
  copyStringSafe(snBuffer, sizeof(snBuffer), appConfig.sn);

  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  wm.startConfigPortal(AP_SSID);
  portalActive = true;
}

void onConnectedToNetwork() {
  if (networkReady || WiFi.status() != WL_CONNECTED) {
    return;
  }

  networkReady = true;
  reconnectAttemptInProgress = false;

  if (!mainServerStarted) {
    server.begin();
    mainServerStarted = true;
  }

  if (!mdnsStarted) {
    if (MDNS.begin(appConfig.host)) {
      MDNS.addService("http", "tcp", 80);
      mdnsStarted = true;
      Serial.print("[mDNS] Disponivel em ");
      Serial.println(currentHostUrl());
    } else {
      Serial.println("[mDNS] Falha ao iniciar mDNS.");
    }
  }

  Serial.print("[WiFi] Conectado. IP: ");
  Serial.println(WiFi.localIP());
}

void onDisconnectedFromNetwork() {
  if (mainServerStarted) {
    server.stop();
    mainServerStarted = false;
  }

  if (mdnsStarted) {
    MDNS.close();
    mdnsStarted = false;
  }

  if (networkReady) {
    Serial.println("[WiFi] Conexao perdida, tentando reconectar...");
  }

  networkReady = false;

  if (!portalActive && !reconnectAttemptInProgress) {
    beginStationConnection();
  }
}

void processButton() {
  if (digitalRead(btnPin) == LOW) {
    if (millis() - lastButtonPress > 400) {
      switch (velocidade) {
        case 0:
          Serial.println("[BTN] Potencia maxima");
          definesaida(3);
          break;
        case 3:
          Serial.println("[BTN] Potencia media");
          definesaida(2);
          break;
        case 2:
          Serial.println("[BTN] Potencia minima");
          definesaida(1);
          break;
        case 1:
          Serial.println("[BTN] Desligando");
          definesaida(0);
          break;
      }
      lastButtonPress = millis();
    }
    yield();
  }
}

void processWiFiState() {
  if (portalActive) {
    wm.process();

    if (shouldSaveConfig) {
      shouldSaveConfig = false;
      appConfig.forcePortalOnBoot = false;
      copyParametersFromPortal();
      saveConfig();
      Serial.println("[WiFi] Dados recebidos pelo portal. Reiniciando para aplicar...");
      scheduleRestart(1500);
    }

    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    onConnectedToNetwork();
    return;
  }

  onDisconnectedFromNetwork();

  if (reconnectAttemptInProgress &&
      millis() - wifiConnectStartedAt >= WIFI_CONNECT_TIMEOUT_MS) {
    beginConfigPortal("falha ao conectar na rede salva");
  }
}

void sendHtmlStart() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html; charset=UTF-8", "");
}

void sendHtmlEnd() {
  server.sendContent("");
}

void sendContent(const String& content) {
  server.sendContent(content);
}

void handleFaviconSvg() {
  static const char faviconSvg[] PROGMEM =
    "<svg xmlns='http://www.w3.org/2000/svg' xml:space='preserve' width='256' height='256' "
    "style='shape-rendering:geometricPrecision;text-rendering:geometricPrecision;image-rendering:optimizeQuality;fill-rule:evenodd;clip-rule:evenodd'>"
    "<circle cx='127.16' cy='128.06' r='128' style='fill:#fff'/>"
    "<path d='M138.2 169.33c-.22 2.45-.49 4.9-.89 7.36-2.7 16.98-10.92 27.05-15.25 41.82-5.16 17.88 12.42 25.58 27.41 24.58 7.63-.67 15.11-2.6 22.14-5.66a85 85 0 0 0 17.58-10.11c37.94-27.92 30.19-65.84-11.53-87.45a28 28 0 0 0-7.81-2.65 41.58 41.58 0 0 1-11.32 20.89 41.8 41.8 0 0 1-20.33 11.22m-9.23-72.97c17.81 0 32.26 14.45 32.26 32.26s-14.45 32.26-32.26 32.26-32.26-14.45-32.26-32.26 14.45-32.26 32.26-32.26m40.86 23.79c2.38.09 4.77.26 7.15.55 17.09 1.88 27.51 9.73 42.44 13.39 18.12 4.43 25.04-13.53 23.42-28.47a70.6 70.6 0 0 0-6.71-21.82 85.6 85.6 0 0 0-10.85-17.14c-29.58-36.74-67.11-27.31-86.88 15.38a26.5 26.5 0 0 0-1.88 5.55 41.6 41.6 0 0 1 21.88 11.52 41.84 41.84 0 0 1 11.32 21zm-50.74-32.07c0-3.02.24-6.06.58-9.08 1.89-17.11 9.66-27.53 13.21-42.48 4.5-18.13-13.49-24.99-28.42-23.33a70.6 70.6 0 0 0-21.82 6.73 84.6 84.6 0 0 0-17.09 10.9c-36.62 29.66-27.15 67.17 15.5 86.84 2.26 1.03 4.65 1.76 7.09 2.15 3.39-15.63 15.41-27.96 30.95-31.73m-31.13 48.27c-.41 0-.86.01-1.28.01-3.17 0-6.36-.15-9.51-.45-17.12-1.66-27.67-9.29-42.67-12.7-18.19-4.15-24.81 13.93-22.95 28.84a71 71 0 0 0 7.06 21.72 85.7 85.7 0 0 0 11.15 16.98c30.18 36.15 67.53 26.16 86.6-16.81.7-1.62 1.27-3.31 1.67-5.03-15.42-4.2-27.11-16.85-30.07-32.56' style='fill:#000'/>"
    "</svg>";

  server.sendHeader("Cache-Control", "public, max-age=604800");
  server.send_P(200, "image/svg+xml", faviconSvg);
}

void renderHomePage() {
  sendHtmlStart();

  sendContent(F(
    "<!DOCTYPE html><html lang='pt-BR'><head>"
    "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>Controle do Ventilador</title>"
    "<link rel='icon' type='image/svg+xml' href='/favicon.svg'>"
    "<link rel='shortcut icon' href='/favicon.svg'>"
    "<link rel='apple-touch-icon' href='/favicon.svg'>"
    "<meta name='theme-color' content='#181828'>"
    "<style>"
    "body{background:#f5f6fa;font-family:Arial,sans-serif;margin:0;padding:24px;color:#0b3056;transition:.2s}"
    ".wrap{max-width:360px;margin:0 auto;position:relative}"
    ".box{background:#fff;border-radius:16px;padding:24px;box-shadow:0 4px 24px rgba(0,0,0,.12);transition:.2s}"
    "h1{text-align:center;margin:0 0 18px}"
    ".status{background:#eef3ff;border-radius:10px;padding:12px;text-align:center;margin-bottom:16px}"
    ".hostinfo{margin-top:-4px;margin-bottom:16px;font-size:.92em;text-align:center;color:#4f6382}"
    ".btn{display:block;text-decoration:none;color:#fff;padding:14px;border-radius:10px;font-weight:700;margin-bottom:12px}"
    ".btn-row{display:flex;align-items:center;justify-content:center;gap:12px}"
    ".btn-icon{display:flex;align-items:center;justify-content:center;width:28px;height:28px;flex:0 0 28px}"
    ".btn svg{width:28px;height:28px;display:block}"
    ".btn-text{display:inline-block}"
    ".max{background:#0EA615}.med{background:#18a0be}.min{background:#8360c3}.off{background:#e84118}"
    ".footer{text-align:center;margin-top:16px;color:#666;font-size:.9em}"
    ".footer a,.wifilink a{color:#377dff;text-decoration:none}"
    ".wifilink{text-align:center;margin-top:14px;font-size:.95em}"
    ".darkbtn{position:absolute;top:10px;right:10px;width:42px;height:42px;border:none;border-radius:50%;background:#eef3ff;color:#0b3056;font-size:18px;cursor:pointer}"
    ".dark body{background:#181828}.dark{background:#181828;color:#fff}.dark .box{background:#232336;color:#fff}"
    ".dark .status{background:#2f3248;color:#fff}.dark .hostinfo{color:#b8c4da}.dark .footer{color:#aaa}"
    ".dark .footer a,.dark .wifilink a{color:#8ab4f8}.dark .darkbtn{background:#2f3248;color:#fff}"
    "</style></head><body><div class='wrap'>"
    "<button id='darkModeBtn' class='darkbtn' title='Alternar tema'>&#9680;</button>"
    "<div class='box'><h1>Controle do Ventilador</h1>"
    "<div class='status'>Estado atual: <strong>"
  ));

  switch (velocidade) {
    case 1:
      sendContent(F("Pot&#234;ncia M&#237;nima"));
      break;
    case 2:
      sendContent(F("Pot&#234;ncia M&#233;dia"));
      break;
    case 3:
      sendContent(F("Pot&#234;ncia M&#225;xima"));
      break;
    default:
      sendContent(F("Desligado"));
      break;
  }

  sendContent(F(
    "</strong></div><div class='hostinfo'>Acesso local: "
  ));
  sendContent(currentHostUrl());
  sendContent(F(
    "</div>"
    "<a href='/?f=MAXIMA' class='btn max'><span class='btn-row'><span class='btn-icon'>"
    "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'><path fill='#fff' d='M0 256a256 256 0 1 1 512 0A256 256 0 1 1 0 256zM288 96a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zM256 416c35.3 0 64-28.7 64-64c0-3.7-.3-7.3-.9-10.8l117.5-72.8c11.3-7 14.7-21.8 7.8-33s-21.8-14.8-33-7.8L293.8 300.4C283.2 292.6 270.1 288 256 288c-35.3 0-64 28.7-64 64s28.7 64 64 64zM176 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zM96 288a32 32 0 1 0 0-64 32 32 0 1 0 0 64zM400 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0z'/></svg>"
    "</span><span class='btn-text'>Pot&#234;ncia M&#225;xima</span></span></a>"
    "<a href='/?f=MEDIA' class='btn med'><span class='btn-row'><span class='btn-icon'>"
    "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'><path fill='#fff' d='M0 256a256 256 0 1 1 512 0A256 256 0 1 1 0 256zm320 96c0-26.9-16.5-49.9-40-59.3L280 88c0-13.3-10.7-24-24-24s-24 10.7-24 24l0 204.7c-23.5 9.5-40 32.5-40 59.3c0 35.3 28.7 64 64 64s64-28.7 64-64zM144 176a32 32 0 1 0 0-64 32 32 0 1 0 0 64zm-16 80a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zm288 32a32 32 0 1 0 0-64 32 32 0 1 0 0 64zM400 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0z'/></svg>"
    "</span><span class='btn-text'>Pot&#234;ncia M&#233;dia</span></span></a>"
    "<a href='/?f=MINIMA' class='btn min'><span class='btn-row'><span class='btn-icon'>"
    "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'><path fill='#fff' d='M0 256a256 256 0 1 1 512 0A256 256 0 1 1 0 256zM288 96a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zM256 416c35.3 0 64-28.7 64-64s-28.7-64-64-64c-14.1 0-27.2 4.6-37.8 12.4L100.6 227.6c-11.3-7-26.1-3.5-33 7.8s-3.5 26.1 7.8 33l117.5 72.8c-.6 3.5-.9 7.1-.9 10.8c0 35.3 28.7 64 64 64zM176 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zM416 288a32 32 0 1 0 0-64 32 32 0 1 0 0 64zM400 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0z'/></svg>"
    "</span><span class='btn-text'>Pot&#234;ncia M&#237;nima</span></span></a>"
    "<a href='/?f=OFF' class='btn off'><span class='btn-row'><span class='btn-icon'>"
    "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'><path fill='#fff' d='M288 32c0-17.7-14.3-32-32-32s-32 14.3-32 32l0 224c0 17.7 14.3 32 32 32s32-14.3 32-32l0-224zM143.5 120.6c13.6-11.3 15.4-31.5 4.1-45.1s-31.5-15.4-45.1-4.1C49.7 115.4 16 181.8 16 256c0 132.5 107.5 240 240 240s240-107.5 240-240c0-74.2-33.8-140.6-86.6-184.6c-13.6-11.3-33.8-9.4-45.1 4.1s-9.4 33.8 4.1 45.1c38.9 32.3 63.5 81 63.5 135.4c0 97.2-78.8 176-176 176s-176-78.8-176-176c0-54.4 24.7-103.1 63.5-135.4z'/></svg>"
    "</span><span class='btn-text'>Desligar</span></span></a>"
    "<div class='footer'>Desenvolvido por <a href='https://github.com/juliolobo/Controle-de-Ventilador-Arduino' target='_blank'>Julio Lobo</a></div>"
    "</div><div class='wifilink'><a href='/wifi'>Configurar Wi-Fi / Trocar rede</a></div></div>"
    "<script>const b=document.body,k='fanDarkMode',t=document.getElementById('darkModeBtn');function a(v){if(v){b.classList.add('dark');localStorage.setItem(k,'1');}else{b.classList.remove('dark');localStorage.setItem(k,'0');}}a(localStorage.getItem(k)==='1');t.onclick=function(){a(!b.classList.contains('dark'));};</script>"
    "</body></html>"
  ));

  sendHtmlEnd();
}

void renderWifiPage(const String& message, bool showStartButton) {
  sendHtmlStart();
  sendContent(F(
    "<!DOCTYPE html><html lang='pt-BR'><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>Configuracao Wi-Fi</title>"
    "<style>body{font-family:Arial,sans-serif;background:#f5f6fa;margin:0;padding:24px;color:#0b3056}"
    ".wrap{max-width:420px;margin:0 auto}.box{background:#fff;border-radius:16px;padding:24px;box-shadow:0 4px 24px rgba(0,0,0,.12)}"
    "h1{margin-top:0;text-align:center}.msg{background:#eef3ff;border-radius:10px;padding:14px;line-height:1.45}"
    ".btn{display:block;text-align:center;text-decoration:none;color:#fff;background:#377dff;padding:14px;border-radius:10px;font-weight:700;margin-top:16px}"
    ".btn.secondary{background:#6b7280}.hint{font-size:.94em;color:#555;margin-top:16px;line-height:1.45}"
    "</style></head><body><div class='wrap'><div class='box'><h1>Configuracao Wi-Fi</h1><div class='msg'>"
  ));
  sendContent(message);
  sendContent(F("</div>"));

  if (showStartButton) {
    sendContent(F("<a class='btn' href='/wifi/start'>Abrir portal de configuracao</a>"));
  }

  sendContent(F(
    "<a class='btn secondary' href='/'>Voltar ao controle</a>"
    "<div class='hint'>Ao abrir o portal, o NodeMCU reinicia e cria a rede <strong>Ventilador-Config</strong>. "
    "Depois conecte o celular nela para escolher o SSID, senha, IP fixo opcional e o nome do host <strong>.local</strong>.</div>"
    "</div></div></body></html>"
  ));
  sendHtmlEnd();
}

void handleRoot() {
  if (server.hasArg("f")) {
    String action = server.arg("f");

    if (action == "MAXIMA") {
      Serial.println("[WEB] Potencia maxima");
      definesaida(3);
    } else if (action == "MEDIA") {
      Serial.println("[WEB] Potencia media");
      definesaida(2);
    } else if (action == "MINIMA") {
      Serial.println("[WEB] Potencia minima");
      definesaida(1);
    } else if (action == "OFF") {
      Serial.println("[WEB] Desligando");
      definesaida(0);
    }
  }

  renderHomePage();
}

void handleWifiPage() {
  String message = "O nome atual para acesso local e <strong>" + currentHostUrl() + "</strong>.<br>"
                   "Clique abaixo para reiniciar no modo de configuracao Wi-Fi.";
  renderWifiPage(message, true);
}

void handleWifiStart() {
  appConfig.forcePortalOnBoot = true;
  saveConfig();

  renderWifiPage(
    "Reiniciando no modo de configuracao.<br>"
    "Em alguns segundos conecte-se na rede <strong>Ventilador-Config</strong> e abra "
    "<strong>http://192.168.4.1</strong> caso o portal nao abra sozinho.",
    false
  );

  scheduleRestart(1500);
}

void handleNotFound() {
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void setup() {
  pinMode(btnPin, INPUT_PULLUP);
  pinMode(conmin, OUTPUT);
  pinMode(conmed, OUTPUT);
  pinMode(conmax, OUTPUT);

  desligarReles();
  velocidade = 0;

  Serial.begin(115200);
  Serial.println();
  Serial.println("[Boot] Ventilador inicializando...");

  loadConfig();
  configureRoutes();
  configureWiFiManager();

  if (appConfig.forcePortalOnBoot) {
    appConfig.forcePortalOnBoot = false;
    saveConfig();
    beginConfigPortal("portal solicitado manualmente");
  } else {
    beginStationConnection();
  }
}

void loop() {
  processButton();
  processWiFiState();

  if (networkReady && mainServerStarted) {
    server.handleClient();
  }

  if (mdnsStarted) {
    MDNS.update();
  }

  if (restartScheduled && static_cast<long>(millis() - restartAt) >= 0) {
    delay(50);
    ESP.restart();
  }
}
