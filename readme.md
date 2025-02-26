# BeatTime

### Um relógio digital interativo com integração ao Spotify e animações LED

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
1. Instale o **Pico SDK**
2. Compile o código:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```
3. Envie o arquivo `.uf2` gerado para a Raspberry Pi Pico W

### **3. Rodar o Servidor Node.js**
Se estiver utilizando o backend para Spotify, inicie o servidor:
```bash
node server.js
```

## 📊 Status do Projeto
✅ Sincronização NTP funcionando  
✅ Exibição do Spotify funcionando  
✅ Controle de reprodução por botões  
✅ LED reage ao áudio  
🔄 Melhorias futuras: otimização de consumo energético e novas animações LED

## 📜 Licença
Este projeto está licenciado sob a **MIT License**. Sinta-se livre para contribuir!

## 🤝 Contribuição
Pull requests são bem-vindos! Se deseja sugerir melhorias, abra uma **issue**.

---
💡 Desenvolvido por Lawtrel

