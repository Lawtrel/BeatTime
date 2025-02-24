#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/ip_addr.h"
#include "lwip/udp.h"

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define NTP_PORT 123
#define NTP_MSG_LEN 48
#define NTP_DELTA 2208988800 // Segundos entre 1 Jan 1900 e 1 Jan 1970

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

// Função para imprimir continuamente o horário atual com incremento local
static void print_continuous_time() {
    while (true) {
        if (last_epoch_time > 0) {
            last_epoch_time++; // Incrementa o tempo local manualmente a cada segundo
            struct tm *utc = gmtime(&last_epoch_time);
            printf("Horário: %02d/%02d/%04d %02d:%02d:%02d\\r",
                   utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
                   utc->tm_hour, utc->tm_min, utc->tm_sec);
            fflush(stdout);
        }
        sleep_ms(1000); // Atualiza a cada segundo
    }
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

// Conecta ao próximo servidor NTP disponível
static void connect_to_ntp_servers(struct udp_pcb *pcb) {
    ip_addr_t server_address;
    for (int i = 0; i < sizeof(ntp_servers) / sizeof(ntp_servers[0]); i++) {
        if (ipaddr_aton(ntp_servers[i], &server_address)) {
            printf("Conectando ao servidor NTP: %s\\n", ntp_servers[i]);
            ntp_request(pcb, &server_address);
            sleep_ms(5000); // Tempo para receber resposta
            if (last_epoch_time > 0) return; // Sai se o horário foi obtido
        } else {
            printf("Endereço IP inválido: %s\\n", ntp_servers[i]);
        }
    }
    printf("Falha ao obter horário de todos os servidores NTP.br.\\n");
}

// Inicializa o Wi-Fi e obtém o horário NTP
int main() {
    stdio_init_all();
    if (cyw43_arch_init()) {
        printf("Falha na inicialização Wi-Fi.\\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar no Wi-Fi.\\n");
        return 1;
    }

    struct udp_pcb *pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        printf("Falha ao criar PCB UDP.\\n");
        return 1;
    }
    udp_recv(pcb, ntp_recv, NULL);

    connect_to_ntp_servers(pcb); // Conecta aos servidores NTP.br

    print_continuous_time(); // Exibe o horário continuamente

    cyw43_arch_deinit();
    return 0;
}