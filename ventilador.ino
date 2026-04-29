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
void desligarReles();
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
  
  if(req.indexOf("favicon.ico") != -1){
      client.stop();
      return;
  }
  
  while (client.connected() && client.available()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break;
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
                 "<title>Controle do Ventilador</title>"
                 "<style>body{background:#f5f6fa;font-family:Arial,sans-serif;margin:0;padding:24px;color:#0b3056;transition:.2s}.wrap{max-width:360px;margin:0 auto;position:relative}.box{background:#fff;border-radius:16px;padding:24px;box-shadow:0 4px 24px rgba(0,0,0,.12);transition:.2s}h1{text-align:center;margin:0 0 18px}.status{background:#eef3ff;border-radius:10px;padding:12px;text-align:center;margin-bottom:16px}.btn{display:block;text-align:center;text-decoration:none;color:#fff;padding:14px;border-radius:10px;font-weight:700;margin-bottom:12px}.max{background:#16C61E}.med{background:#18a0be}.min{background:#8360c3}.off{background:#e84118}.footer{text-align:center;margin-top:16px;color:#666;font-size:.9em}.darkbtn{position:absolute;top:10px;right:10px;width:42px;height:42px;border:none;border-radius:50%;background:#eef3ff;color:#0b3056;font-size:18px;cursor:pointer}.dark body{background:#181828}.dark{background:#181828;color:#fff}.dark .box{background:#232336;color:#fff}.dark .status{background:#2f3248;color:#fff}.dark .footer{color:#aaa}.dark .darkbtn{background:#2f3248;color:#fff}</style>"
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
                 "<a href='?f=MAXIMA' class='btn max'>Potência Máxima</a>"
                 "<a href='?f=MEDIA' class='btn med'>Potência Média</a>"
                 "<a href='?f=MINIMA' class='btn min'>Potência Mínima</a>"
                 "<a href='?f=OFF' class='btn off'>Desligar</a>"
                 "<div class='footer'>Desenvolvido por Julio Lobo</div>"
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
