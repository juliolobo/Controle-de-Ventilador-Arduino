# Controle-de-Ventilador-NodeMCU

Controle um ventilador de 3 velocidades usando relés e um `NodeMCU ESP8266`, com:

- botão físico para alternar as velocidades
- interface web para controle pelo navegador
- portal de configuração Wi-Fi
- suporte a hostname local, como `http://ventilador.local`
- opção de IP fixo ou DHCP

## Como funciona

Substitua a chave seletora original do ventilador por:

- 1 push button
- 3 saídas de relé, uma para cada velocidade

Sequência do botão físico:

1. desligado -> potência máxima
2. potência máxima -> potência média
3. potência média -> potência mínima
4. potência mínima -> desligado

O ventilador sempre inicia desligado.

## Materiais utilizados

1. Push Button
2. NodeMCU v2 / ESP8266
3. Módulo relé de 4 canais
4. Fonte de alimentação 5V

## Bibliotecas necessárias

Instale no `Arduino IDE`:

1. `WiFiManager` by `tzapu/tablatronix`
2. `ArduinoJson` by `Benoit Blanchon`

Observações:

- `LittleFS`, `ESP8266WebServer` e `ESP8266mDNS` já fazem parte do core do `ESP8266`.
- Certifique-se de ter a placa `ESP8266` instalada no `Boards Manager` do Arduino IDE.

## Primeira configuração do Wi-Fi

Na primeira vez que o NodeMCU for ligado, ou sempre que ele não conseguir se conectar à rede salva, ele entrará automaticamente em modo de configuração.

### Passo a passo

1. Ligue o NodeMCU.
2. No celular ou computador, conecte-se à rede Wi-Fi criada pelo módulo:
   - `Ventilador-Config`
3. Se o portal não abrir automaticamente, abra no navegador:
   - `http://192.168.4.1`
4. Escolha o SSID da sua rede Wi-Fi.
5. Digite a senha da rede.
6. Preencha, se quiser:
   - `Nome do Host (sem .local)`
   - `IP local`
   - `Gateway`
   - `Subnet`
7. Salve a configuração.
8. O NodeMCU reiniciará e tentará se conectar à rede configurada.

## Acesso pelo navegador

Depois de conectado ao Wi-Fi, você pode acessar a interface web usando:

- o IP mostrado no `Monitor Serial`
- ou o hostname configurado no portal, por exemplo:
  - `http://ventilador.local`
  - `http://ventiladordasala.local`

Observação:

- O acesso via `.local` depende de suporte a `mDNS` no dispositivo usado para acessar.

## IP fixo ou DHCP

O portal permite dois modos:

- deixar os campos `IP local`, `Gateway` e `Subnet` vazios para usar `DHCP`
- preencher esses campos para usar `IP fixo`

Isso e útil em redes onde o DHCP do roteador apresenta incompatibilidade com o `ESP8266`.

## Trocar a rede depois

Na interface principal existe um link para:

- `Configurar Wi-Fi / Trocar rede`

Ao acessar essa opção, o módulo reinicia em modo de configuração para permitir cadastrar uma nova rede.

## Interface web

A interface principal possui:

- botões para potência máxima, média, mínima e desligar
- dark mode
- ícones SVG nos botões
- favicon
- link para o repositório no rodapé

## Observações importantes

- Ajuste a pinagem no arquivo `ventilador.ino` se necessário.
- Este projeto não é compatível com ventiladores controlados por dimmer.
- Tome cuidado com a parte elétrica e com o isolamento dos relés.

## Imagens

Ao acessar o endereço do módulo no navegador, você verá uma interface semelhante a esta:

<img width="387" height="576" alt="image" src="https://github.com/user-attachments/assets/4a218ada-26a2-4145-a27e-2368fc87a21b" />
<img width="387" height="574" alt="image" src="https://github.com/user-attachments/assets/37d8a590-8ea5-4afd-a1e8-dd1a8cec1611" />
