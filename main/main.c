// Projeto final modulo II - Estufa
// Franzininho WiFi LAB01

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_console.h"
#include "ssd1306.h"
#include "sensores.h"
#include "arquivo_log.h"

// botoes da LAB01
#define BT_CIMA   2
#define BT_BAIXO  3
#define BT_OK     4
#define BT_VOLTAR 5

SSD1306_t oled;

// valores dos sensores
float temperatura = 0;
float umidade = 0;
int luz = 0;

// configuracoes
int setpoint = CONFIG_SETPOINT_PADRAO;
int controle_ligado = 1;
int registro_ligado = 1;
int saida = 0;

// menu
int no_menu = 0;
int item = 0;      // 0 = setpoint, 1 = controle, 2 = registro
int editando = 0;

// guarda o estado anterior de cada botao
int estado_anterior[50];

// comando "log" do terminal
int comando_log(int argc, char **argv)
{
    log_mostrar();
    return 0;
}

void iniciar_terminal()
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "estufa>";
    esp_console_dev_usb_cdc_config_t cdc_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    esp_console_new_repl_usb_cdc(&cdc_config, &repl_config, &repl);

    esp_console_register_help_command();
    esp_console_cmd_t cmd = {
        .command = "log",
        .help = "mostra o arquivo log.txt",
        .func = &comando_log
    };
    esp_console_cmd_register(&cmd);

    esp_console_start_repl(repl);
}

void iniciar_botoes()
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << BT_CIMA) | (1ULL << BT_BAIXO) | (1ULL << BT_OK) | (1ULL << BT_VOLTAR),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&io);
}

// retorna 1 quando o botao acabou de ser apertado
int apertou(int pino)
{
    int agora = gpio_get_level(pino);
    int resultado = 0;
    if (agora == 0 && estado_anterior[pino] == 1) {
        resultado = 1;
    }
    estado_anterior[pino] = agora;
    return resultado;
}

// escreve uma linha no display (16 letras por linha)
void escrever(int linha, char *texto, int invertido)
{
    char buf[17];
    snprintf(buf, sizeof(buf), "%-16s", texto);
    ssd1306_display_text(&oled, linha, buf, 16, invertido);
}

void mostrar_tela()
{
    char txt[20];

    if (no_menu == 0) {
        escrever(0, "ESTUFA", 0);
        sprintf(txt, "Temp: %.1f C", temperatura);
        escrever(2, txt, 0);
        sprintf(txt, "Umid: %.0f %%", umidade);
        escrever(3, txt, 0);
        sprintf(txt, "Luz:  %d %%", luz);
        escrever(4, txt, 0);
        sprintf(txt, "Setpoint: %d C", setpoint);
        escrever(5, txt, 0);
        if (controle_ligado) {
            sprintf(txt, "Saida: %s", saida ? "LIGADA" : "DESLIG");
        } else {
            sprintf(txt, "Controle: OFF");
        }
        escrever(6, txt, 0);
        sprintf(txt, "Log:%s  OK=menu", registro_ligado ? "ON " : "OFF");
        escrever(7, txt, 0);
    } else {
        escrever(0, "MENU", 0);
        sprintf(txt, "Setpoint: %d", setpoint);
        escrever(2, txt, item == 0);
        sprintf(txt, "Controle: %s", controle_ligado ? "ON" : "OFF");
        escrever(3, txt, item == 1);
        sprintf(txt, "Registro: %s", registro_ligado ? "ON" : "OFF");
        escrever(4, txt, item == 2);
        if (editando) {
            escrever(6, "CIMA/BAIXO muda", 0);
            escrever(7, "OK = pronto", 0);
        } else {
            escrever(6, "OK = alterar", 0);
            escrever(7, "VOLTAR = sair", 0);
        }
    }
}

void app_main(void)
{
    // inicia tudo
    gpio_reset_pin(CONFIG_PINO_SAIDA);
    gpio_set_direction(CONFIG_PINO_SAIDA, GPIO_MODE_OUTPUT);
    iniciar_botoes();
    sensores_iniciar(CONFIG_PINO_DHT, CONFIG_CANAL_LDR);
    log_iniciar();

    vTaskDelay(pdMS_TO_TICKS(300)); // espera o display ligar
    i2c_master_init(&oled, 8, 9, -1);
    ssd1306_init(&oled, 128, 64);
    ssd1306_clear_screen(&oled, false);

    iniciar_terminal();

    int contador = 0;

    while (1) {
        // le os botoes
        int cima = apertou(BT_CIMA);
        int baixo = apertou(BT_BAIXO);
        int ok = apertou(BT_OK);
        int voltar = apertou(BT_VOLTAR);

        // menu
        if (no_menu == 0) {
            if (ok) {
                no_menu = 1;
                item = 0;
                ssd1306_clear_screen(&oled, false);
            }
        } else if (editando) {
            if (cima && setpoint < 50) setpoint++;
            if (baixo && setpoint > 0) setpoint--;
            if (ok || voltar) editando = 0;
        } else {
            if (cima) item--;
            if (baixo) item++;
            if (item < 0) item = 2;
            if (item > 2) item = 0;
            if (ok) {
                if (item == 0) editando = 1;
                if (item == 1) controle_ligado = !controle_ligado;
                if (item == 2) registro_ligado = !registro_ligado;
            }
            if (voltar) {
                no_menu = 0;
                ssd1306_clear_screen(&oled, false);
            }
        }

        // a cada 2 segundos le os sensores e faz o controle
        if (contador % 20 == 0) {
            sensores_ler(&temperatura, &umidade, &luz);

            if (controle_ligado) {
                // liga abaixo do setpoint e desliga acima (1 grau de folga)
                if (temperatura < setpoint - 1) saida = 1;
                if (temperatura > setpoint + 1) saida = 0;
            } else {
                saida = 0;
            }
            gpio_set_level(CONFIG_PINO_SAIDA, saida);
        }

        // grava no log.txt
        if (registro_ligado && contador % (CONFIG_INTERVALO_LOG * 10) == 0) {
            log_gravar(temperatura, umidade, luz, setpoint, saida);
        }

        mostrar_tela();

        contador++;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
