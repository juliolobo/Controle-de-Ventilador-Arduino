/**********************************************************
* Ventilador Contolado por 1  push button                 *
* Versão 3.0/2025  By JulioLobo                           *
*                                                         *
* Esse projeto ser usado em qualquer ventilador que tenha *
* uma chave selecionadora de velocidade com 3 velocidades *
* Não é compatível com ventiladores com DIMMER            *
*                                                         *
* Troque a chave seletora de velocidade do ventilador por *
* um push button, e 3 pinos de saída, um para cada relê.  * 
* O esquema de ligação está junto deste código            *
*                                                         *
* Você é livre para modificar e usar esse código, só peço *
* que mantenha os créditos, obrigado por respeitar isso!  *
***********************************************************/

#include <ESP8266WiFi.h>
const char* ssid = "NOME_DA_REDE_WIFI"; 
const char* password = "SENHA_DA_REDE_WIFI";

//IP do NodeMCU (para voce acessar pelo browser - voce TEM que mudar este IP tambem)
// IPAddress ip(192, 168, 1, 203);
//IP do roteador da sua rede wifi
// IPAddress gateway(192, 168, 1, 1);
//Mascara de rede da sua rede wifi
// IPAddress subnet(255, 255, 255, 0);

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
const int conmin = 12; //Pino de controle do relê potência mínima
const int conmed = 5;  //Puno de controle do relê potência média
const int conmax = 4;  //Pino de controle do relê potência máxima
int velocidade = 0;
void definesaida(int saida);
unsigned long lastButtonPress = 0;

void setup() {
    pinMode(btnPin, INPUT_PULLUP);
    pinMode(conmin, OUTPUT);
    pinMode(conmed, OUTPUT);
    pinMode(conmax, OUTPUT);
    velocidade = 0;

    Serial.begin(115200);

    //Conectando a rede Wifi
    // WiFi.config(ip, gateway, subnet);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.println("Conectando ao WiFi..");
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
      delay(10);
    }

  //Verificando se o servidor esta pronto.
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  //Verificando se o servidor recebeu alguma requisicao
  while (!client.available()) {
    delay(1);
  }

  //Obtendo a requisicao vinda do browser
  String req = client.readStringUntil('\r');
  
  //Sugestao dada por Enrico Orlando
  if(req == "GET /favicon.ico HTTP/1.1"){
      req = client.readStringUntil('\r');
  }
  
  client.flush();

  //Iniciando o buffer que ira conter a pagina HTML que sera enviada para o browser.
  String buf = "";
  buf += "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
  buf += "<!DOCTYPE html><html lang='pt-BR'><head>";
  buf += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  buf += "<title>Controle do Ventilador</title>";
  buf += "<style>";
  buf += "body{background:#f5f6fa;font-family:'Segoe UI','Arial',sans-serif;margin:0;padding:0;}";
  buf += ".center{display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh;}";
  buf += ".box{background:#fff;border-radius:16px;box-shadow:0 4px 24px #b9bcc459;padding:0px 28px 24px;max-width:330px;width:100%;}";
  buf += "h1{color:#0b3056;font-size:2em;margin-bottom:24px;text-align:center;letter-spacing:-1px;}";
  buf += ".btn{display:flex;align-items:center;justify-content:center;margin-bottom:20px;width:100%;padding:16px 0 16px 0;font-size:1.15em;color:#fff;border:none;border-radius:8px;cursor:pointer;transition:background 0.2s;font-weight:500;box-shadow:0 2px 8px #b9bcc459;}";
  buf += ".btn:last-child{margin-bottom:0;}";
  buf += ".btn-icon{display:flex;align-items:center;justify-content:center;min-width:32px;max-width:32px;width:32px;height:32px;margin-right:12px;}";
  buf += ".btn-text{width:160px;text-align:left;}";
  buf += ".btn svg{width:28px;height:28px;display:block;}";
  buf += ".btn-max{background:#fa7e1e;}";
  buf += ".btn-max:hover{background:#f96c1e;}";
  buf += ".btn-med{background:#18a0be;}";
  buf += ".btn-med:hover{background:#118db6;}";
  buf += ".btn-min{background:#8360c3;}";
  buf += ".btn-min:hover{background:#6640a3;}";
  buf += ".btn-off{background:#e84118;}";
  buf += ".btn-off:hover{background:#c23616;}";
  buf += ".footer{margin-top:16px;color:#b4b4b4;font-size:.9em;text-align:center;}";
  buf += "a{color:#377dff;text-decoration:none;}";
  buf += ".box{position:relative;}"; // já no seu código? Se não, adicione!
  buf += "body.dark{background:#181828;}";
  buf += ".box.dark{background:#232336;}";
  buf += "body.dark h1{color:#fff;}";
  buf += "body.dark .btn{color:#fff;}";
  buf += "body.dark .footer{color:#888;}";
  buf += "body.dark .btn-max{background:#eb861e;}";
  buf += "body.dark .btn-med{background:#119fba;}";
  buf += "body.dark .btn-min{background:#6e53b8;}";
  buf += "body.dark a{color:#8ab4f8;}";
  buf += "</style></head><body>";
  buf += "<button id='darkModeBtn' title='Modo Escuro' style='position:absolute;top:18px;right:18px;background:transparent;border:none;cursor:pointer;outline:none;'><svg id='iconSun' style='display:none;' xmlns='http://www.w3.org/2000/svg' width='22' height='22' viewBox='0 0 24 24'><circle cx='12' cy='12' r='5' fill='#FFDF00'/><g stroke='#FFDF00' stroke-width='2'><line x1='12' y1='2' x2='12' y2='5'/><line x1='12' y1='19' x2='12' y2='22'/><line x1='2' y1='12' x2='5' y2='12'/><line x1='19' y1='12' x2='22' y2='12'/><line x1='4.2' y1='4.2' x2='6.3' y2='6.3'/><line x1='17.7' y1='17.7' x2='19.8' y2='19.8'/><line x1='4.2' y1='19.8' x2='6.3' y2='17.7'/><line x1='17.7' y1='6.3' x2='19.8' y2='4.2'/></g></svg><svg id='iconMoon' xmlns='http://www.w3.org/2000/svg' width='22' height='22' viewBox='0 0 24 24'><path fill='#444' d='M21 12.79A9 9 0 0 1 12.79 3a1 1 0 0 0-1.1 1.36A7 7 0 1 0 19.64 12.9a1 1 0 0 0 1.36-1.1z'/></svg></button>";
  buf += "<div class='center'><div class='box'>";
  buf += "<h1>Controle do Ventilador</h1>";
  buf += "<a href='?f=FORTE' class='btn btn-max' type='submit' title='Forte'>";
  buf += "<span class='btn-icon'>";
  buf += "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'>";
  buf += "<path fill='#fff' d='M0 256a256 256 0 1 1 512 0A256 256 0 1 1 0 256zM288 96a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zM256 416c35.3 0 64-28.7 64-64c0-3.7-.3-7.3-.9-10.8l117.5-72.8c11.3-7 14.7-21.8 7.8-33s-21.8-14.8-33-7.8L293.8 300.4C283.2 292.6 270.1 288 256 288c-35.3 0-64 28.7-64 64s28.7 64 64 64zM176 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zM96 288a32 32 0 1 0 0-64 32 32 0 1 0 0 64zM400 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0z'/>";
  buf += "</svg>";
  buf += "</span>";
  buf += "<span class='btn-text'>Potência Máxima</span>";
  buf += "</a>";
  buf += "<a href='?f=MEDIO' class='btn btn-med' type='submit' title='Médio'>";
  buf += "<span class='btn-icon'>";
  buf += "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'>";
  buf += "<path fill='#fff' d='M0 256a256 256 0 1 1 512 0A256 256 0 1 1 0 256zm320 96c0-26.9-16.5-49.9-40-59.3L280 88c0-13.3-10.7-24-24-24s-24 10.7-24 24l0 204.7c-23.5 9.5-40 32.5-40 59.3c0 35.3 28.7 64 64 64s64-28.7 64-64zM144 176a32 32 0 1 0 0-64 32 32 0 1 0 0 64zm-16 80a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zm288 32a32 32 0 1 0 0-64 32 32 0 1 0 0 64zM400 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0z'/>";
  buf += "</svg>";
  buf += "</span>";
  buf += "<span class='btn-text'>Potência Média</span>";
  buf += "</a>";
  buf += "<a href='?f=FRACO' class='btn btn-min' type='submit' title='Fraca'>";
  buf += "<span class='btn-icon'>";
  buf += "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'>";
  buf += "<path fill='#fff' d='M0 256a256 256 0 1 1 512 0A256 256 0 1 1 0 256zM288 96a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zM256 416c35.3 0 64-28.7 64-64s-28.7-64-64-64c-14.1 0-27.2 4.6-37.8 12.4L100.6 227.6c-11.3-7-26.1-3.5-33 7.8s-3.5 26.1 7.8 33l117.5 72.8c-.6 3.5-.9 7.1-.9 10.8c0 35.3 28.7 64 64 64zM176 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0zM416 288a32 32 0 1 0 0-64 32 32 0 1 0 0 64zM400 144a32 32 0 1 0 -64 0 32 32 0 1 0 64 0z'/>";
  buf += "</svg>";
  buf += "</span>";
  buf += "<span class='btn-text'>Potência Mínima</span>";
  buf += "</a>";
  buf += "<a href='?f=OFF' class='btn btn-off' type='submit' title='Desligar'>";
  buf += "<span class='btn-icon'>";
  buf += "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 512 512'>";
  buf += "<path fill='#fff' d='M288 32c0-17.7-14.3-32-32-32s-32 14.3-32 32l0 224c0 17.7 14.3 32 32 32s32-14.3 32-32l0-224zM143.5 120.6c13.6-11.3 15.4-31.5 4.1-45.1s-31.5-15.4-45.1-4.1C49.7 115.4 16 181.8 16 256c0 132.5 107.5 240 240 240s240-107.5 240-240c0-74.2-33.8-140.6-86.6-184.6c-13.6-11.3-33.8-9.4-45.1 4.1s-9.4 33.8 4.1 45.1c38.9 32.3 63.5 81 63.5 135.4c0 97.2-78.8 176-176 176s-176-78.8-176-176c0-54.4 24.7-103.1 63.5-135.4z'/>";
  buf += "</svg>";
  buf += "</span>";
  buf += "<span>Desligar</span>";
  buf += "</a>";
  buf += "<div class='footer'>Desenvolvido por <a href='https://github.com/juliolobo/Controle-de-Ventilador-Arduino' target='_blank'>Julio Lobo</a></div>";
  buf += "</div></div>";
  buf += "<script>";
  buf += "const btn = document.getElementById('darkModeBtn');";
  buf += "const iconSun = document.getElementById('iconSun');";
  buf += "const iconMoon = document.getElementById('iconMoon');";
  buf += "function setDarkMode(e){";
  buf += "if(e){document.body.classList.add('dark');document.querySelector('.box').classList.add('dark');iconSun.style.display='inline';iconMoon.style.display='none';localStorage.setItem('darkmode','1');}";
  buf += "else{document.body.classList.remove('dark');document.querySelector('.box').classList.remove('dark');iconSun.style.display='none';iconMoon.style.display='inline';localStorage.setItem('darkmode','0');}";
  buf += "}";
  buf += "window.onload=function(){";
  buf += "if(localStorage.getItem('darkmode')==='1'){setDarkMode(true);}else{setDarkMode(false);}";
  buf += "};";
  buf += "btn.onclick=function(){setDarkMode(!document.body.classList.contains('dark'));};";
  buf += "</script>";
  buf += "</body></html>";
  //Enviando para o browser a 'pagina' criada.
  client.print(buf);
  client.flush();

    //Analisando a requisicao recebida para decidir se liga ou desliga a lampada
    if (req.indexOf("FORTE") != -1){
        Serial.println("Definindo potencia maxima"); //Somente para debug
        definesaida(3);
    }else if (req.indexOf("MEDIO") != -1){
        Serial.println("Definindo potencia media"); //Somente para debug
        definesaida(2);
    }else if (req.indexOf("FRACO") != -1){
        Serial.println("Definindo potencia minima"); //Somente para debug
        definesaida(1);
    }else if (req.indexOf("OFF") != -1){
        Serial.println("Desligando"); //Somente para debug
        definesaida(0);
    }
}

void definesaida(int saida) //Função que define o estado do ventilador
{
  switch (saida) {
    case 0:
      digitalWrite(conmax, LOW);
      digitalWrite(conmed, LOW);
      digitalWrite(conmin, LOW);
      velocidade = 0;
      break;

    case 1:
      digitalWrite(conmax, LOW);
      digitalWrite(conmed, LOW);
      digitalWrite(conmin, HIGH);
      velocidade = 1;
      break;

    case 2:
      digitalWrite(conmax, LOW);
      digitalWrite(conmin, LOW);
      digitalWrite(conmed, HIGH);
      velocidade = 2;
      break;

    case 3:
      digitalWrite(conmin, LOW);
      digitalWrite(conmed, LOW);
      digitalWrite(conmax, HIGH);
      velocidade = 3;
      break;
  }
  delay(500);
}
