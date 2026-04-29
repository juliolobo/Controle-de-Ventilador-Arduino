/**********************************************************
* Ventilador Contolado por 1 push button                  *
* Versão 4.0/2026 - Desenvolvido por JulioLobo            *
*                                                         *
* Esse projeto pode ser usado em ventiladores que tenham  *
* uma chave seletora de velocidade com 3 velocidades      *
* Não é compatível com ventiladores com DIMMER            *
*                                                         *
* Troque a chave seletora de velocidade do ventilador por *
* um push button.                                         *
* Usei um NodeMCU com um módulo de 4 relés.               *
* Os 3 pinos que iriam para a chave usei 1 em cada relé.  *
* O esquema de ligação está junto deste código            *
*                                                         *
* Você é livre para modificar e usar esse código, só peço *
* que mantenha os créditos, obrigado por respeitar isso!  *
***********************************************************/

#include <ESP8266WiFi.h>
const char* ssid = "NOME_DA_REDE_WIFI"; 
const char* password = "SENHA_DA_REDE_WIFI";

//IP do NodeMCU (para voce acessar pelo browser - voce TEM que mudar este IP tambem)
IPAddress ip(192, 168, 1, 155);
//IP do roteador da sua rede wifi
IPAddress gateway(192, 168, 1, 1);
//Mascara de rede da sua rede wifi
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(80);

/* -----------------------------------------
| NodeMCU / ESP8266  |  NodeMCU / ESP8266  |
| D0 = 16            |  D6 = 12            |
| D1 = 5             |  D7 = 13            |
| D2 = 4             |  D8 = 15            |
| D3 = 0             |  D9 = 3             |
| D4 = 2             |  D10 = 1            |
| D5 = 14            |                     |
------------------------------------------*/

const int btnPin = 14; //Pino que recebe o push button
const int conmin = 12; //Pino de controle do relé potência mínima
const int conmed = 5;  //Pino de controle do relé potência média
const int conmax = 4;  //Pino de controle do relé potência máxima
const unsigned long tempoTrocaRele = 180;
int velocidade = 0;
void definesaida(int saida);
void enviarFaviconSvg(WiFiClient &client)
{
  client.print(F("HTTP/1.1 200 OK\r\nContent-Type: image/svg+xml\r\nCache-Control: public, max-age=604800\r\nConnection: close\r\n\r\n"
                 "<svg xmlns='http://www.w3.org/2000/svg' xml:space='preserve' width='256' height='256' style='shape-rendering:geometricPrecision;text-rendering:geometricPrecision;image-rendering:optimizeQuality;fill-rule:evenodd;clip-rule:evenodd'><circle cx='127.16' cy='128.06' r='128' style='fill:#fff'/><path d='M138.2 169.33c-.22 2.45-.49 4.9-.89 7.36-2.7 16.98-10.92 27.05-15.25 41.82-5.16 17.88 12.42 25.58 27.41 24.58 7.63-.67 15.11-2.6 22.14-5.66a85 85 0 0 0 17.58-10.11c37.94-27.92 30.19-65.84-11.53-87.45a28 28 0 0 0-7.81-2.65 41.58 41.58 0 0 1-11.32 20.89 41.8 41.8 0 0 1-20.33 11.22m-9.23-72.97c17.81 0 32.26 14.45 32.26 32.26s-14.45 32.26-32.26 32.26-32.26-14.45-32.26-32.26 14.45-32.26 32.26-32.26m40.86 23.79c2.38.09 4.77.26 7.15.55 17.09 1.88 27.51 9.73 42.44 13.39 18.12 4.43 25.04-13.53 23.42-28.47a70.6 70.6 0 0 0-6.71-21.82 85.6 85.6 0 0 0-10.85-17.14c-29.58-36.74-67.11-27.31-86.88 15.38a26.5 26.5 0 0 0-1.88 5.55 41.6 41.6 0 0 1 21.88 11.52 41.84 41.84 0 0 1 11.32 21zm-50.74-32.07c0-3.02.24-6.06.58-9.08 1.89-17.11 9.66-27.53 13.21-42.48 4.5-18.13-13.49-24.99-28.42-23.33a70.6 70.6 0 0 0-21.82 6.73 84.6 84.6 0 0 0-17.09 10.9c-36.62 29.66-27.15 67.17 15.5 86.84 2.26 1.03 4.65 1.76 7.09 2.15 3.39-15.63 15.41-27.96 30.95-31.73m-31.13 48.27c-.41 0-.86.01-1.28.01-3.17 0-6.36-.15-9.51-.45-17.12-1.66-27.67-9.29-42.67-12.7-18.19-4.15-24.81 13.93-22.95 28.84a71 71 0 0 0 7.06 21.72 85.7 85.7 0 0 0 11.15 16.98c30.18 36.15 67.53 26.16 86.6-16.81.7-1.62 1.27-3.31 1.67-5.03-15.42-4.2-27.11-16.85-30.07-32.56' style='fill:#000'/></svg>"));
}

void desligarReles();
void enviarFaviconSvg(WiFiClient &client);
unsigned long lastButtonPress = 0;

void setup() {
    pinMode(btnPin, INPUT_PULLUP);
    pinMode(conmin, OUTPUT);
    pinMode(conmed, OUTPUT);
    pinMode(conmax, OUTPUT);
    desligarReles();
    velocidade = 0;

    Serial.begin(115200);

    //Conectando a rede Wifi
    WiFi.config(ip, gateway, subnet);
    WiFi.begin(ssid, password);
    unsigned long wifiTimeout = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Conectando ao WiFi..");
        if (millis() - wifiTimeout > 20000) {
            ESP.restart();
        }
    }
    Serial.println("Conectado!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
    //Iniciando o servidor Web
    server.begin();
}
void loop(){
    //Controlando pelo Push button
    if(digitalRead(btnPin) == LOW){
      if (millis() - lastButtonPress > 400) {
        switch (velocidade) {
            case 0:
                Serial.println("Definindo potencia maxima"); //Somente para debug
                definesaida(3);
                break;
            case 3:
                Serial.println("Definindo potencia media"); //Somente para debug
                definesaida(2);
                break;
            case 2:
                Serial.println("Definindo potencia minima"); //Somente para debug
                definesaida(1);
                break;
            case 1:
                Serial.println("Desligando"); //Somente para debug
                definesaida(0);
                break;
        }
        lastButtonPress = millis();
      }
      yield();
    }

  //Verificando se o servidor esta pronto.
  WiFiClient client = server.available();
  if (client) client.setNoDelay(true);
  if (!client) {
    return;
  }

  //Verificando se o servidor recebeu alguma requisicao
  unsigned long timeout = millis();
  while (!client.available()) {
      if (millis() - timeout > 1000) {
          client.stop();
          return;
      }
      delay(1);
  }

  //Obtendo a requisicao vinda do browser
  String req = client.readStringUntil('\r');
  
  while (client.connected() && client.available()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break;
  }

  if(req.indexOf("GET /favicon.svg") != -1 || req.indexOf("GET /favicon.ico") != -1){
      enviarFaviconSvg(client);
      delay(5);
      client.stop();
      return;
  }

  //Analisando a requisicao recebida para decidir se liga ou desliga o ventilador
  if (req.indexOf("MAXIMA") != -1){
      Serial.println("Definindo potencia maxima"); //Somente para debug
      definesaida(3);
  }else if (req.indexOf("MEDIA") != -1){
      Serial.println("Definindo potencia media"); //Somente para debug
      definesaida(2);
  }else if (req.indexOf("MINIMA") != -1){
      Serial.println("Definindo potencia minima"); //Somente para debug
      definesaida(1);
  }else if (req.indexOf("OFF") != -1){
      Serial.println("Desligando"); //Somente para debug
      definesaida(0);
  }

  client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: close\r\n\r\n"
                 "<!DOCTYPE html><html lang='pt-BR'><head>"
                 "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>"
                 "<title>Controle do Ventilador</title><link rel='icon' type='image/svg+xml' href='/favicon.svg'><link rel='shortcut icon' href='/favicon.svg'><link rel='apple-touch-icon' href='/favicon.svg'><meta name='theme-color' content='#181828'>"
                 "<style>body{background:#f5f6fa;font-family:Arial,sans-serif;margin:0;padding:24px;color:#0b3056;transition:.2s}.wrap{max-width:360px;margin:0 auto;position:relative}.box{background:#fff;border-radius:16px;padding:24px;box-shadow:0 4px 24px rgba(0,0,0,.12);transition:.2s}h1{text-align:center;margin:0 0 18px}.status{background:#eef3ff;border-radius:10px;padding:12px;text-align:center;margin-bottom:16px}.btn{display:block;text-decoration:none;color:#fff;padding:14px;border-radius:10px;font-weight:700;margin-bottom:12px}.btn-row{display:flex;align-items:center;justify-content:center;gap:12px}.btn-icon{display:flex;align-items:center;justify-content:center;width:28px;height:28px;flex:0 0 28px}.btn svg{width:28px;height:28px;display:block}.btn-text{display:inline-block}.max{background:#0EA615}.med{background:#18a0be}.min{background:#8360c3}.off{background:#e84118}.footer{text-align:center;margin-top:16px;color:#666;font-size:.9em}.footer a{color:#377dff;text-decoration:none}.darkbtn{position:absolute;top:10px;right:10px;width:42px;height:42px;border:none;border-radius:50%;background:#eef3ff;color:#0b3056;font-size:18px;cursor:pointer}.dark body{background:#181828}.dark{background:#181828;color:#fff}.dark .box{background:#232336;color:#fff}.dark .status{background:#2f3248;color:#fff}.dark .footer{color:#aaa}.dark .footer a{color:#8ab4f8}.dark .darkbtn{background:#2f3248;color:#fff}</style>"
                 "</head><body><div class='wrap'><button id='darkModeBtn' class='darkbtn' title='Alternar tema'>◐</button><div class='box'><h1>Controle do Ventilador</h1><div class='status'>Estado atual: <strong>"));

  switch (velocidade) {
    case 1:
      client.print(F("Mínima"));
      break;
    case 2:
      client.print(F("Média"));
      break;
    case 3:
      client.print(F("Máxima"));
      break;
    default:
      client.print(F("Desligado"));
      break;
  }

  client.print(F("</strong></div>"
                 "<a href='?f=MAXIMA' class='btn max'><span class='btn-row'><span class='btn-icon'><svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'><path fill='#fff' d='M0 256a256 256 0 1 1 512 0A256 256 0 1 1 0 256zM288 96a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zM256 416c35.3 0 64-28.7 64-64c0-3.7-.3-7.3-.9-10.8l117.5-72.8c11.3-7 14.7-21.8 7.8-33s-21.8-14.8-33-7.8L293.8 300.4C283.2 292.6 270.1 288 256 288c-35.3 0-64 28.7-64 64s28.7 64 64 64zM176 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zM96 288a32 32 0 1 0 0-64 32 32 0 1 0 0 64zM400 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0z'/></svg></span><span class='btn-text'>Potência Máxima</span></span></a>"
                 "<a href='?f=MEDIA' class='btn med'><span class='btn-row'><span class='btn-icon'><svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'><path fill='#fff' d='M0 256a256 256 0 1 1 512 0A256 256 0 1 1 0 256zm320 96c0-26.9-16.5-49.9-40-59.3L280 88c0-13.3-10.7-24-24-24s-24 10.7-24 24l0 204.7c-23.5 9.5-40 32.5-40 59.3c0 35.3 28.7 64 64 64s64-28.7 64-64zM144 176a32 32 0 1 0 0-64 32 32 0 1 0 0 64zm-16 80a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zm288 32a32 32 0 1 0 0-64 32 32 0 1 0 0 64zM400 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0z'/></svg></span><span class='btn-text'>Potência Média</span></span></a>"
                 "<a href='?f=MINIMA' class='btn min'><span class='btn-row'><span class='btn-icon'><svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'><path fill='#fff' d='M0 256a256 256 0 1 1 512 0A256 256 0 1 1 0 256zM288 96a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zM256 416c35.3 0 64-28.7 64-64s-28.7-64-64-64c-14.1 0-27.2 4.6-37.8 12.4L100.6 227.6c-11.3-7-26.1-3.5-33 7.8s-3.5 26.1 7.8 33l117.5 72.8c-.6 3.5-.9 7.1-.9 10.8c0 35.3 28.7 64 64 64zM176 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zM416 288a32 32 0 1 0 0-64 32 32 0 1 0 0 64zM400 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0z'/></svg></span><span class='btn-text'>Potência Mínima</span></span></a>"
                 "<a href='?f=OFF' class='btn off'><span class='btn-row'><span class='btn-icon'><svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'><path fill='#fff' d='M288 32c0-17.7-14.3-32-32-32s-32 14.3-32 32l0 224c0 17.7 14.3 32 32 32s32-14.3 32-32l0-224zM143.5 120.6c13.6-11.3 15.4-31.5 4.1-45.1s-31.5-15.4-45.1-4.1C49.7 115.4 16 181.8 16 256c0 132.5 107.5 240 240 240s240-107.5 240-240c0-74.2-33.8-140.6-86.6-184.6c-13.6-11.3-33.8-9.4-45.1 4.1s-9.4 33.8 4.1 45.1c38.9 32.3 63.5 81 63.5 135.4c0 97.2-78.8 176-176 176s-176-78.8-176-176c0-54.4 24.7-103.1 63.5-135.4z'/></svg></span><span class='btn-text'>Desligar</span></span></a>"
                 "<div class='footer'>Desenvolvido por <a href='https://github.com/juliolobo/Controle-de-Ventilador-Arduino' target='_blank'>Julio Lobo</a></div>"
                 "</div></div><script>const b=document.body,k='fanDarkMode',t=document.getElementById('darkModeBtn');function a(v){if(v){b.classList.add('dark');localStorage.setItem(k,'1');}else{b.classList.remove('dark');localStorage.setItem(k,'0');}}a(localStorage.getItem(k)==='1');t.onclick=function(){a(!b.classList.contains('dark'));};</script></body></html>"));

  yield();
  delay(5);
  client.stop();
}

void desligarReles()
{
  digitalWrite(conmax, LOW);
  digitalWrite(conmed, LOW);
  digitalWrite(conmin, LOW);
}

void definesaida(int saida) //Função que define o estado do ventilador
{
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
