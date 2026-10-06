#include <stdio.h>
#include "arquivo_log.h"
#include "esp_spiffs.h"
#include "esp_timer.h"

void log_iniciar(void)
{
    // monta o SPIFFS na particao "storage"
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 2,
        .format_if_mount_failed = true
    };
    esp_vfs_spiffs_register(&conf);
}

void log_gravar(float temperatura, float umidade, int luz, int setpoint, int saida)
{
    FILE *f = fopen("/spiffs/log.txt", "a");
    if (f == NULL) {
        printf("erro ao abrir o log.txt\n");
        return;
    }
    // tempo em segundos desde que ligou
    int segundos = esp_timer_get_time() / 1000000;
    fprintf(f, "%ds T=%.1f U=%.1f L=%d%% SP=%d saida=%d\n", segundos, temperatura, umidade, luz, setpoint, saida);
    fclose(f);
}

void log_mostrar(void)
{
    FILE *f = fopen("/spiffs/log.txt", "r");
    if (f == NULL) {
        printf("log vazio\n");
        return;
    }
    char linha[80];
    while (fgets(linha, sizeof(linha), f) != NULL) {
        printf("%s", linha);
    }
    fclose(f);
}
