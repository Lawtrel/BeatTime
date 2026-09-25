# BeatTime

**Projeto de sistemas embarcados em C: relógio, display OLED, entrada de áudio e integração HTTP com um backend Node.js.**

Este repositório contém o firmware para Raspberry Pi Pico W. O servidor que intermedeia a integração com Spotify fica em [BeatTime-Server](https://github.com/Lawtrel/BeatTime-Server).

## Mapa do projeto

- `BeatTime.c`: programa principal e integração dos periféricos.
- `ssd1306_i2c.c` / `ssd1306_i2c.h`: comunicação com o display.
- `ws2818b.pio`: programa PIO utilizado pelo CMake para controle dos LEDs.
- `CMakeLists.txt`: alvo `BeatTime`, placa `pico_w` e bibliotecas do Pico SDK.

A parte de automação demonstrada é a integração de entradas, saídas, comunicação e lógica embarcada.

## 📌 Sobre o Projeto
O **BeatTime** é um dispositivo baseado no Raspberry Pi Pico W que combina funcionalidades de um relógio digital, exibição de músicas do Spotify e uma matriz de LEDs que reage ao som ambiente. O projeto integra:
- Sincronização horária via **NTP**
- Exibição da música atual via **API do Spotify**
- Controle de reprodução (avançar/retroceder) por botões físicos
- Animação de LEDs baseada no nível de áudio captado pelo microfone

## 🎛 Componentes Utilizados
- **Microcontrolador:** Raspberry Pi Pico W
- **Display:** OLED SSD1306
- **Iluminação:** Matriz de 25 LEDs WS2812B
- **Entradas:** Botões físicos e microfone
- **Conectividade:** Wi-Fi integrado

## 🛠 Tecnologias Utilizadas
- **Linguagem de Programação:** C
- **APIs:** Spotify API, NTP
- **Servidor Backend:** Node.js (para autenticação e comunicação com a API do Spotify)
- **Bibliotecas:**
  - `ssd1306` (controle do display OLED)
  - `lwip` (conexão de rede)
  - `pico-sdk` (interação com o hardware)
  - `neopixel` (controle da matriz de LEDs)

## 🔄 Fluxo de Funcionamento
1. O dispositivo inicializa e conecta ao Wi-Fi
2. Obtém o horário via NTP e exibe no OLED
3. Faz requisições periódicas para a API do Spotify para exibir a música atual
4. Responde a botões físicos para avançar/retroceder faixas
5. Captura áudio ambiente e controla os LEDs em resposta ao som

## 🚀 Como Usar
### **1. Configurar Wi-Fi e API do Spotify**
Edite o arquivo `BeatTime.c` e substitua as credenciais:
```c
#define WIFI_SSID "SuaRedeWiFi"
#define WIFI_PASSWORD "SuaSenha"
#define API_SERVER "192.168.x.x" // Endereço do servidor Node.js
```

### **2. Compilar e Subir para a Raspberry Pi Pico W**
1. Instale o **Pico SDK**, CMake e a toolchain ARM. O CMake do projeto referencia SDK 2.1.1; configure `PICO_SDK_PATH` para a instalação local.
2. Compile o código:
   ```bash
   cmake -S . -B build -DPICO_BOARD=pico_w
   cmake --build build
   ```
3. Envie o arquivo `.uf2` gerado para a Raspberry Pi Pico W

### **3. Rodar o Servidor Node.js**
O servidor é um repositório separado:

```bash
git clone https://github.com/Lawtrel/BeatTime-Server.git
cd BeatTime-Server
npm install
```

Crie um `.env` local com `SPOTIFY_CLIENT_ID`, `SPOTIFY_CLIENT_SECRET` e `SPOTIFY_REDIRECT_URI`, conforme o aplicativo cadastrado no Spotify. Não versione credenciais. Inicie com:

```bash
node server.js
```

O servidor utiliza a porta 5000 e expõe `/login`, `/callback`, `/spotify`, `/spotify/previous` e `/spotify/next`. Configure o endereço do servidor no firmware. A integração depende da autorização da conta e dos recursos disponíveis para o aplicativo no Spotify.

## 📊 Status do Projeto
Protótipo educacional com implementação de sincronização NTP, integração Spotify, botões e reação dos LEDs ao áudio. Uma revisão documental não substitui a validação no dispositivo.

Roteiro para reproduzir a demonstração:

1. Compilar e registrar a versão do SDK e a placa utilizada.
2. Conferir o horário NTP e a exibição no OLED.
3. Verificar o microfone e a resposta dos LEDs.
4. Autorizar a conta no servidor e testar a música atual e os botões.
5. Testar perda de Wi-Fi, servidor indisponível e expiração da autorização.

Próximas melhorias: documentar a montagem e os pinos, revisar tratamento de falhas e renovação de token no servidor, e registrar vídeo curto do hardware com os resultados do roteiro.

## 📜 Licença
Este projeto está licenciado sob a **MIT License**. Sinta-se livre para contribuir!

## 🤝 Contribuição
Pull requests são bem-vindos! Se deseja sugerir melhorias, abra uma **issue**.

---
💡 Desenvolvido por Lawtrel

