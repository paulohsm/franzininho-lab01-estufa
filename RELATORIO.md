# Relatorio Tecnico - Estufa

## O que foi usado do curso

- **GPIO:** botoes como entrada (com pull-up) e LED como saida.
- **ADC:** leitura do LDR no ADC1. O ESP32-S2 tem ADC de 13 bits (0 a 8191), entao o valor foi convertido para porcentagem.
- **DHT11:** leitura de temperatura e umidade com a biblioteca usada no curso.
- **I2C e display OLED:** biblioteca ssd1306 para mostrar os valores e o menu.
- **SPIFFS:** particao "storage" na tabela de particoes e arquivo log.txt.
- **Console serial:** comando `log` feito com o esp_console.
- **menuconfig:** arquivo Kconfig.projbuild com pinos, setpoint inicial e intervalo do log.
- **Componentes:** a leitura dos sensores e o log ficaram em componentes separados do main.

## Como o programa funciona

O app_main inicia tudo e entra num loop que roda a cada 100 ms. Em cada volta ele le os botoes, trata o menu e atualiza o display. A cada 2 segundos le os sensores e faz o controle. No intervalo configurado grava uma linha no log.txt, se o registro estiver ligado.

## Escolhas

- **Loop unico:** mais simples de entender e suficiente para este projeto.
- **Botoes lidos no loop:** o programa compara o estado atual com o anterior para saber quando o botao foi apertado. Como o loop roda a cada 100 ms, isso ja evita o repique.
- **Controle com folga de 1 grau:** a saida liga abaixo de setpoint - 1 e desliga acima de setpoint + 1, para nao ficar ligando e desligando o tempo todo.
- **SPIFFS:** ja vem no ESP-IDF e foi o que vimos no curso.
- **Tempo do log em segundos desde que a placa ligou:** a placa nao tem relogio.

## Limitacoes

- As configuracoes feitas no menu voltam ao padrao quando a placa reinicia.
- O DHT11 tem pouca precisao (1 grau).
- O log nao tem limite de tamanho.
