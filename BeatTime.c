#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "ws2818b.pio.h"
#include "neopixel.c"

#define WIFI_SSID "Start Fibra - Le 2G"
#define WIFI_PASSWORD "LeKinho03@"
#define NTP_PORT 123
#define NTP_MSG_LEN 48
#define NTP_DELTA 2208988800

#define WS2812_PIN 7
#define NUM_LED 25
#define BUZZER 21
#define LED_GREEN 11
#define LED_BLUE 12
#define LED_RED 13
#define MIC_CHANNEL 2
#define MIC_PIN (26 + MIC_CHANNEL)
#define NUM_SAMPLES  100  // Número de amostras para calcular a média
#define ADC_MAX      4095  // Máximo valor do ADC (12 bits no RP2040)
#define ADC_REF_VOLT 3.3   // Tensão de referência do ADC (3.3V)

#define API_SERVER "192.168.0.100"
#define API_PORT 5000
#define API_PATH "/spotify"

//Display OLED
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
    "200.186.125.195"
};

time_t last_epoch_time = 0; // Armazena o horário atual
absolute_time_t last_sync_time; // Armazena o horário da última sincronização
char spotify_track[64] = "Carregando...";
int scroll_position = 0;
int scroll_speed = 3; // velocidade de rolagem
struct tcp_pcb *pcb;
ip_addr_t server_ip;

void setup();
void init_display();
void update_display(const char *message);
void update_oled_display();
void sync_ntp();
void configure_dns();
void fetch_spotify_track();
float get_microphone_level();
void update_led_matrix();

static void update_time() {
    int64_t elapsed_ms = absolute_time_diff_us(last_sync_time, get_absolute_time()) / 1000;
    last_epoch_time += elapsed_ms / 1000;
    last_sync_time = delayed_by_ms(last_sync_time, (elapsed_ms / 1000) * 1000);
}

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
        if (pbuf_copy_partial(p, buf, 4, 40) == 4) {
            time_t epoch = ((uint32_t)buf[0] << 24 | (uint32_t)buf[1] << 16 |
                            (uint32_t)buf[2] << 8 | (uint32_t)buf[3]) - NTP_DELTA - (3600 * 3);
            if (epoch > 0) {
                last_epoch_time = epoch;
                last_sync_time = get_absolute_time();
                printf("Horário NTP sincronizado.\n");
            }
        }
    }
    pbuf_free(p);
}

// Callback para processar resposta do servidor
static err_t spotify_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p) {
        char response[512] = {0};
        pbuf_copy_partial(p, response, sizeof(response) - 1, 0);
        response[p->len] = '\0';
        
        // Extrair apenas os dados da resposta
        char *track_start = strstr(response, "\r\n\r\n");
        if (track_start) {
            track_start += 4;
            snprintf(spotify_track, sizeof(spotify_track), "%s", track_start);
        } else {
            snprintf(spotify_track, sizeof(spotify_track), "Erro API");
        }
        pbuf_free(p);
    } else {
        snprintf(spotify_track, sizeof(spotify_track), "Sem resposta");
    }
    tcp_close(tpcb);
    pcb = NULL;
    return ERR_OK;
}

// Callback para confirmar conexão e enviar requisição
static err_t spotify_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err != ERR_OK) {
        printf("Erro na conexão TCP: %d\n", err);
        tcp_close(tpcb);
        return err;
    }
    char request[256];
    snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", API_PATH, API_SERVER);
    tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_recv(tpcb, spotify_recv_callback);
    return ERR_OK;
}
void test_microphone() {
    float mic_level = get_microphone_level();
    printf("Nível do microfone: %f\n", mic_level);
    sleep_ms(500);
}
// Inicializa o Wi-Fi e obtém o horário NTP
int main() {
    stdio_init_all();
    setup();
    init_display();

    while (true) {
        update_time();
        fetch_spotify_track();
        test_microphone();
        update_led_matrix();
        update_oled_display();
        sleep_ms(100);
    }
    cyw43_arch_deinit();
    return 0;
}
//Configurar os Botoes e Joystick
void setup() {
    if (cyw43_arch_init()) return;
    cyw43_arch_enable_sta_mode();
    cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000);
    sync_ntp();
    configure_dns();
    sync_ntp();
    configure_dns();
    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);

    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_init(LED_BLUE);
    gpio_set_dir(LED_BLUE, GPIO_OUT);

    npInit(WS2812_PIN, NUM_LED);
    adc_gpio_init(MIC_PIN);
    adc_init();
    adc_select_input(MIC_CHANNEL);

    adc_fifo_setup(
      true, // Habilitar FIFO
      true, // Habilitar request de dados do DMA
      1, // Threshold para ativar request DMA é 1 leitura do ADC
      false, // Não usar bit de erro
      false // Não fazer downscale das amostras para 8-bits, manter 12-bits.
    );
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
    struct tm *local_time = localtime(&last_epoch_time);
    char buffer[20];  
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
    memset(ssd1306_buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(ssd1306_buffer, 30, 10, buffer);
    // Efeito de rolagem
    int max_char_por_linha = ssd1306_width / 9;
    char linha[20] = {0};
    strncpy(linha, spotify_track + scroll_position, max_char_por_linha);
    linha[max_char_por_linha] = '\0';
    // Exibir a faixa do Spotify
    ssd1306_draw_string(ssd1306_buffer, 5, 20, linha);
    // Atualizar a posição de rolagem
    scroll_position += scroll_speed;
    if (scroll_position > strlen(spotify_track)) {
        scroll_position = 0; // Reinicia a posição de rolagem quando a mensagem termina
    }
        render_on_display(ssd1306_buffer, &frame_area);
}

void sync_ntp() {
    struct udp_pcb *pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        printf("❌ Erro ao criar socket UDP para NTP.\n");
        return;
    }
    udp_recv(pcb, ntp_recv, NULL);
    ip_addr_t server_address;
    if (ipaddr_aton(ntp_servers[0], &server_address)) {
        ntp_request(pcb, &server_address);
        sleep_ms(5000);
    }
}

void configure_dns() {
    ip4addr_aton(API_SERVER, &server_ip);
}

// Obtém a faixa atual do Spotify
void fetch_spotify_track() {    
    if (pcb) {
        tcp_abort(pcb);
        pcb = NULL;
    }
    
    pcb = tcp_new();
    if (!pcb) {
        printf("❌ Erro ao criar conexão TCP\n");
        return;
    }

    if (tcp_connect(pcb, &server_ip, API_PORT, spotify_connected_callback) != ERR_OK) {
        printf("⚠️ Erro ao conectar à API\n");
        tcp_abort(pcb);
        pcb = NULL;
    }
}

// Captura o nível do microfone
float get_microphone_level() {
    float mic_signal = 0;
    float mic_average = 0;

    // Captura várias amostras para obter um valor médio
    for (int i = 0; i < NUM_SAMPLES; i++) {
        int adc_value = adc_read(); // Lê o valor bruto do ADC
        mic_average += adc_value;
    }
    mic_average /= NUM_SAMPLES;  // Média das amostras

    // Calcula a variação do sinal (pico AC)
    for (int i = 0; i < NUM_SAMPLES; i++) {
        int adc_value = adc_read();
        mic_signal += fabs(adc_value - mic_average);
    }
    mic_signal /= NUM_SAMPLES; // Média da variação
    
    // Normaliza o nível do microfone para um valor entre 0 e 100
    float mic_level = (mic_signal / ADC_MAX) * 100;
    
    return mic_level;
}

void update_led_matrix() {
    float mic_level = get_microphone_level();
    uint8_t brightness = mic_level * 80;
    if (brightness > 255) brightness = 255;

    npClear();
    int num_leds_on = (brightness * NUM_LED) / 255;

    for (int i = 0; i < num_leds_on; i++) {
        int row = i / 5;  // Linha da matriz
        int col = i % 5;  // Coluna da matriz

        // Ajuste para zig-zag
        if (row % 2 == 0) {
            col = 4 - col;  // Invertemos a ordem da coluna em linhas pares
        }

        int led_index = row * 5 + col;  // Posição correta na matriz

        npSetLED(led_index, brightness, 0, 255 - brightness);
    }

    npWrite();
}