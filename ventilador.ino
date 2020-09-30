/**********************************************************
* Ventilador Contolado por MQTT e 1  push button          *
* Versão 1.0 / 2020  By JulioLobo                         *
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
**********************************************************/

#include <ESP8266WiFi.h>
//#include <PubSubClient.h>
const char* ssid = "SUAREDEWIFI"; 
const char* password =  "**************";

WiFiClient espClient;
/*
-------------------------------------------------
NodeMCU / ESP8266  |  NodeMCU / ESP8266  |
D0 = 16            |  D6 = 12            |
D1 = 5             |  D7 = 13            |
D2 = 4             |  D8 = 15            |
D3 = 0             |  D9 = 3             |
D4 = 2             |  D10 = 1            |
D5 = 14            |                     |
-------------------------------------------------
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
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.println("Conectando ao WiFi..");
    }
    Serial.println("Conectado!");
}
void loop(){
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
