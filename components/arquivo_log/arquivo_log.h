// biblioteca para salvar os dados no arquivo log.txt (SPIFFS)

void log_iniciar(void);
void log_gravar(float temperatura, float umidade, int luz, int setpoint, int saida);
void log_mostrar(void);
