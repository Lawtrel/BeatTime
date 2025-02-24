#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "lwip/ip_addr.h"
#include "lwip/udp.h"
#include "ssd1306.h"


#define NTP_PORT 123
#define NTP_MSG_LEN 48
#define NTP_DELTA 2208988800
#define WS2812_PIN 7
#define NUM_LED 25
//Display OLED
PIO pio = pio0;
uint sm = 0;
uint8_t ssd1306_buffer[ssd1306_buffer_length];
struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

// Lista de servidores NTP.br
static const char *ntp_servers[] = {
    "200.160.7.186",
    "201.49.148.135",
    "200.186.125.195",
    "200.20.186.76",
    "200.160.0.8",
    "200.189.40.8",
    "200.192.232.8",
    "200.160.7.193"
};

static time_t last_epoch_time = 0; // Armazena o horário atual

void init_display();
void update_display(const char *message);
void update_oled_display();
void print_continuous_time();
int show_message();
int sync_ntp();
// Função para enviar a requisição NTP
static void ntp_request(struct udp_pcb *pcb, ip_addr_t *server_address) {
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *)p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b; // Configuração padrão da mensagem NTP
    udp_sendto(pcb, p, server_address, NTP_PORT);
    pbuf_free(p);
}

// Função callback para processar a resposta NTP
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    if (p && port == NTP_PORT) {
        uint8_t buf[4];
        // Verificação do deslocamento correto para o timestamp (deve ser 40 para os segundos transmitidos)
        if (pbuf_copy_partial(p, buf, 4, 40) == 4) { // Confirma se 4 bytes foram lidos com sucesso
            time_t epoch = ((uint32_t)buf[0] << 24 | (uint32_t)buf[1] << 16 |
                            (uint32_t)buf[2] << 8 | (uint32_t)buf[3]) - NTP_DELTA - (3600 * 3); // Ajuste para o fuso horário de Brasília
            if (epoch > 0) {
                last_epoch_time = epoch;
                printf("Horário NTP inicial obtido. Iniciando atualização contínua...\\n");
            } else {
                printf("Timestamp NTP inválido recebido.\\n");
            }
        } else {
            printf("Falha ao ler timestamp NTP do buffer.\\n");
        }
    }
    pbuf_free(p);
}

// Inicializa o Wi-Fi e obtém o horário NTP
int main() {
    stdio_init_all();
    init_display();

    if (show_message() == 0) {
        if (sync_ntp() == 0) {
            memset(ssd1306_buffer, 0, ssd1306_buffer_length);
            render_on_display(ssd1306_buffer, &frame_area);
            print_continuous_time();
        }
    }
    cyw43_arch_deinit();
    return 0;
}

// Função para inicializar o display OLED
void init_display() {
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);

    ssd1306_init();
    calculate_render_area_buffer_length(&frame_area);
    memset(ssd1306_buffer, 0, ssd1306_buffer_length);
    render_on_display(ssd1306_buffer, &frame_area);
}

// Função para atualizar o display com uma mensagem personalizada
void update_display(const char *message) {
    memset(ssd1306_buffer, 0, ssd1306_buffer_length);
    int max_char_por_linha = ssd1306_width / 9;
    char linha[20] = {0};
    char linha2[20] = {0};

    if (strlen(message) > max_char_por_linha) {
        strncpy(linha, message, max_char_por_linha);
        linha[max_char_por_linha] = '\0';
        strncpy(linha2, message + max_char_por_linha, max_char_por_linha);
        linha2[max_char_por_linha] = '\0';
    } else {
        strcpy(linha, message);
    }

    ssd1306_draw_string(ssd1306_buffer, 5, 10, linha);
    ssd1306_draw_string(ssd1306_buffer, 5, 20, linha2);
    render_on_display(ssd1306_buffer, &frame_area);
}

// Função para exibir continuamente a hora no OLED
void update_oled_display() {
    if (last_epoch_time > 0) {
        struct tm *local_time = localtime(&last_epoch_time);
        
        char buffer[20];  
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", 
                 local_time->tm_hour, local_time->tm_min, local_time->tm_sec);

        // Atualiza o display com a hora formatada
        update_display(buffer);
    }
}

// Função para imprimir continuamente o horário atual com incremento local
void print_continuous_time() {
    while (true) {
        if (last_epoch_time > 0) {
            last_epoch_time++; // Incrementa o tempo local manualmente a cada segundo
            printf("Horario: %02d:%02d:%02d\r", 
                gmtime(&last_epoch_time)->tm_hour, 
                gmtime(&last_epoch_time)->tm_min, 
                gmtime(&last_epoch_time)->tm_sec);
            update_oled_display(); // Atualiza o display OLED com o horário
            }
        sleep_ms(1000); // Atualiza a cada segundo
    }
}

int show_message() {
    update_display("Inicializando...");
    sleep_ms(2000);

    if (cyw43_arch_init()) {
        update_display("Erro: Wi-Fi falhou!");
        sleep_ms(3000);
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    update_display("Conectando Wi-Fi...");
    sleep_ms(2000);

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        update_display("Erro: Sem Wi-Fi!");
        sleep_ms(3000);
        return 1;
    }
    update_display("Wi-Fi Conectado!");
    sleep_ms(2000);
    return 0;
}
int sync_ntp(){
    struct udp_pcb *pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        update_display("Erro: PCB UDP!");
        sleep_ms(3000);
        return 1;
    }
    udp_recv(pcb, ntp_recv, NULL);
    update_display("Sicronizando");
    sleep_ms(2000);
    // Conecta aos servidores NTP.br
    ip_addr_t server_address;
    for (int i = 0; i < sizeof(ntp_servers) / sizeof(ntp_servers[0]); i++) {
        if (ipaddr_aton(ntp_servers[i], &server_address)) {
            ntp_request(pcb, &server_address);
            sleep_ms(5000); // Tempo para receber resposta
            if (last_epoch_time > 0) {
                update_display("Horario Sicronizado!");
                sleep_ms(2000);
                return 0;
            } // Sai se o horário foi obtido
        }
    }
    update_display("Erro: NTP");
    sleep_ms(3000);
    return 1;
}
