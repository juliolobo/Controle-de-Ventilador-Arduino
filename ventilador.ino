/**********************************************************
* Ventilador Contolado por 1  push button                 *
* Versão 2.0/2020  By JulioLobo                           *
*                                                         *
* Esse projeto ser usado em qualquer ventilador que tenha *
* uma chave selecionadora de velocidade, não é compatível *
* ventiladores controlados por DIMMER.                    *
*                                                         *
* Troque a chave seletora de velocidade do ventilador por *
* um push button, e 3 pinos de saída, um para cada relê.  * 
* O esquema de ligação deve estar junto deste código      *  
* fonte.                                                  *
*                                                         *
* Você é livre para modificar e usar esse código, só peço *
* que mantenha a referência do meu nome nele, obrigado    *
* por respeitar isso !                                    *
***********************************************************/

#include <ESP8266WiFi.h>
const char* ssid = "SEUWIFI"; 
const char* password =  "SENHADOWIFI";
//IP do ESP (para voce acessar pelo browser - voce TEM que mudar este IP tambem)
IPAddress ip(192, 168, 10, 195);

//IP do roteador da sua rede wifi
IPAddress gateway(192, 168, 10, 1);

//Mascara de rede da sua rede wifi
IPAddress subnet(255, 255, 255, 0);

//Criando o servidor web na porta 80
WiFiServer server(80);

/*
------------------------------------------
NodeMCU / ESP8266  |  NodeMCU / ESP8266  |
D0 = 16            |  D6 = 12            |
D1 = 5             |  D7 = 13            |
D2 = 4             |  D8 = 15            |
D3 = 0             |  D9 = 3             |
D4 = 2             |  D10 = 1            |
D5 = 14            |                     |
------------------------------------------
*/

const int btnPin = 14; //Pino que recebe o push button
const int conmin = 12; //Pino de controle do relê potência mínima
const int conmed = 5;  //Puno de controle do relê potência média
const int conmax = 4;  //Pino de controle do relê potência máxima
int velocidade = 0;

void setup() {
    pinMode(btnPin, INPUT_PULLUP);
    pinMode(conmin, OUTPUT);
    pinMode(conmed, OUTPUT);
    pinMode(conmax, OUTPUT);

    Serial.begin(115200);

    //Conectando a rede Wifi
    WiFi.config(ip, gateway, subnet);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.println("Conectando ao WiFi..");
    }
    Serial.println("Conectado!");
    //Iniciando o servidor Web
    server.begin();
}
void loop(){
    //Controlando pelo Push button
    if(digitalRead(btnPin) == LOW){
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
        delay(500);
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
  buf += "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
  buf += "<head> ";
  buf += "<meta charset='UTF-8'> ";
  buf += "<meta http-equiv='cache-control' content='max-age=0' /> ";
  buf += "<meta http-equiv='cache-control' content='no-cache' /> ";
  buf += "<meta http-equiv='expires' content='0' /> ";
  buf += "<meta http-equiv='expires' content='Tue, 01 Jan 1980 1:00:00 GMT' /> ";
  buf += "<meta http-equiv='pragma' content='no-cache' /> ";
  buf += "<title>Controle do Ventilador</title> ";
  buf += "<style> ";
  buf += "body{font-family:Open Sans; color:#555555;} ";
  buf += "h1{font-size:24px; font-weight:normal; margin:0.4em 0;} ";
  buf += ".container { width: 100%; margin: 0 auto; } ";
  buf += ".container .row { float: left; clear: both; width: 100%; } ";
  buf += ".container .col { float: left; margin: 0 0 1.2em; padding-right: 1.2em; padding-left: 1.2em; } ";
  buf += ".container .col.eight{width: 80%;font-weight: bold;} ";
  buf += ".container .col.one{width: 10%;} ";
  buf += ".container .col.four, .container .col.twelve { width: 100%; } ";
  buf += "@media screen and (min-width: 767px) { ";
  buf += ".container{width: 100%; max-width: 1080px; margin: 0 auto;} ";
  buf += ".container .row{width:100%; float:left; clear:both;} ";
  buf += ".container .col{float: left; margin: 0 0 1em; padding-right: .5em; padding-left: .5em;} ";
  buf += ".container .col.four { width: 50%; } ";
  buf += ".container .col.tweleve { width: 100%; } ";
  buf += "} ";
  buf += "* {-moz-box-sizing: border-box; -webkit-box-sizing: border-box; box-sizing: border-box;} ";
  buf += "a{text-decoration:none;} ";
  buf += ".btn {font-size: 18px; white-space:nowrap; width:100%; padding:.8em 1.5em; font-family: Open Sans, Helvetica,Arial,sans-serif; ";
  buf += "line-height:18px; display: inline-block;zoom: 1; color: #fff; text-align: center; position:relative; ";
  buf += "-webkit-transition: border .25s linear, color .25s linear, background-color .25s linear; ";
  buf += "transition: border .25s linear, color .25s linear, background-color .25s linear;} ";
  buf += ".btn.btn-sea{background-color: #08bc9a; border-color: #08bc9a; -webkit-box-shadow: 0 3px 0 #088d74; box-shadow: 0 3px 0 #088d74;} ";
  buf += ".btn.btn-sea:hover{background-color:#01a183;} ";
  buf += ".btn.btn-sea:active{ top: 3px; outline: none; -webkit-box-shadow: none; box-shadow: none;} ";
  buf += "</style> ";
  buf += "</head> ";
  buf += "<body> ";
  buf += "<div class='container'> ";
  buf += "<div class='row'> ";
  buf += "<div class='col twelve'> ";
  buf += "<p align='center'><font size='10'>Controle do Ventilador</font></p> ";
  buf += "</div> ";
  buf += "</div> ";
  buf += "<div class='row'> ";
  buf += "<div class='col one'></div> ";
  buf += "<div class='col eight'> ";
  buf += "<a href='?f=FORTE' class='btn btn-sea'>Forte</a> ";
  buf += "</div> ";
  buf += "</div> ";
  buf += "<div class='row'> ";
  buf += "<div class='col one'></div> ";
  buf += "<div class='col eight'> ";
  buf += "<a href='?f=MEDIO' class='btn btn-sea'>M&eacute;dio</a> ";
  buf += "</div> ";
  buf += "</div> ";
  buf += "<div class='row'> ";
  buf += "<div class='col one'></div> ";
  buf += "<div class='col eight'> ";
  buf += "<a href='?f=FRACO' class='btn btn-sea'>Fraca</a> ";
  buf += "</div> ";
  buf += "</div> ";
  buf += "<div class='row'> ";
  buf += "<div class='col one'></div> ";
  buf += "<div class='col eight'> ";
  buf += "<a href='?f=OFF' class='btn btn-sea'>Desligar</a> ";
  buf += "</div> ";
  buf += "</div> ";
  buf += "<div class='col twelve'> ";
  buf += "<p align='center'><font size='5'><a href='http://lobohost.com.br' target='_blank'>LoboHost.com.br</a></font></p> ";
  buf += "</div> ";
  buf += "</div> ";
  buf += "</body> ";
  buf += "</html> ";
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
