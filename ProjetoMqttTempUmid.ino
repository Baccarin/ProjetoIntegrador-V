

#include <WiFi.h>
#include <PubSubClient.h>
#include <dht.h>

//
//Dados do seu WIFI
const char *ssid = "REDE_WIFI";
const char *password = "SENHA_WIFI";

hw_timer_t *timer = NULL; //faz o controle do temporizador (interrupção por tempo)
//função que o temporizador irá chamar, para reiniciar o ESP32
void IRAM_ATTR resetModule() {
  ets_printf("(watchdog) reiniciar\n"); //imprime no log
  esp_restart(); //reinicia a conecção
}

#define DEBUG                   //Se comentar para a impressão na porta serial

// DADOSDO MQTT
#define servidor_mqtt             "m24.cloudmqtt.com"  //URL do servidor MQTT
#define servidor_mqtt_porta       "PORTA_SERVIDOR"  //Porta do servidor
#define servidor_mqtt_usuario     "USUARIO_SERVIDOR"  //Usuário
#define servidor_mqtt_senha       "SENHA_SERVIDOR"  //Senha
#define mqtt_topico_sub           "REDE"    //Tópico para SUBSCREVER 
#define mqtt_topico_pub           "REDE"    //Tópico para PUBLICAR

#define dht_dpin 13 //Pino DATA do Sensor ligado na porta Analogica

dht DHT; //Inicializa o sensor
WiFiClient espClient;                                 //Instância do WiFiClient
PubSubClient client(espClient);                       //Passando a instância do WiFiClient para a instância do PubSubClient

//função que o configura o temporizador se travar reinicia
void configureWatchdog() {
  timer = timerBegin(0, 80, true); //timerID 0, div 80
  //timer, callback, interrupção de borda
  timerAttachInterrupt(timer, &resetModule, true);
  //timer, tempo (us), repetição
  timerAlarmWrite(timer, 30000000, true);
  timerAlarmEnable(timer); //habilita a interrupção //enable interrupt
}

//Função para imprimir na porta serial
void imprimirSerial(bool linha, String mensagem) {
#ifdef DEBUG
  if (linha) {
    Serial.println(mensagem);
  } else {
    Serial.print(mensagem);
  }
#endif
}

//Função que reconecta ao servidor MQTT
void reconectar() {
  //Repete até conectar
  while (!client.connected()) {
    imprimirSerial(false, "Tentando conectar ao servidor MQTT...");

    //Tentativa de conectar. Se o MQTT precisa de autenticação, será chamada a função com autenticação, caso contrário, chama a sem autenticação.
    bool conectado = strlen(servidor_mqtt_usuario) > 0 ?
                     client.connect("ESP826Client", servidor_mqtt_usuario, servidor_mqtt_senha) :
                     client.connect("ESP826Client");

    if (conectado) {
      imprimirSerial(true, "Conectado!");
      //Subscreve para monitorar os comandos recebidos
      client.subscribe(mqtt_topico_sub, 1); //QoS 1
    } else {
      imprimirSerial(false, "Falhou ao tentar conectar. Codigo: ");
      imprimirSerial(false, String(client.state()).c_str());
      imprimirSerial(true, " tentando novamente em 5 segundos");
      //Aguarda 5 segundos para tentar novamente
      delay(5000);
    }
  }
}
//Função inicial (será executado SOMENTE quando ligar o ESP)
void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  imprimirSerial(true, "...");

  configureWatchdog();
  //inicializa WiFi
  setupWiFi();

  imprimirSerial(false, "IP: ");
  imprimirSerial(true, WiFi.localIP().toString());

  //Informando ao client do PubSub a url do servidor e a porta.
  int portaInt = atoi(servidor_mqtt_porta);
  client.setServer(servidor_mqtt, portaInt);
}

//Função do WIFI
void setupWiFi()
{
  //Coloca como modo station
  WiFi.mode(WIFI_STA);
  //Conecta à rede
  WiFi.begin(ssid, password);
  Serial.println("");

  //Enquanto não conectar
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");

  }

  //Se chegou aqui está conectado
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
}

void tem_umi(){
  DHT.read11(dht_dpin); //Lê as informações do sensor
  Serial.print("Umidade = ");
  Serial.print(DHT.humidity);
  Serial.print(" %  ");
  Serial.print("Temperatura = ");
  Serial.print(DHT.temperature);
  Serial.println(" Celsius  ");
  int temn = DHT.temperature;
  char tem [10];
  int umin = DHT.humidity;
  char umi [10];

  sprintf(tem, "%d", temn);
  sprintf(umi, "%d", umin);
  client.publish("A temperatura atual medida é:","");
  client.publish(tem,"Graus celsius");
  delay (5000);
  client.publish("A umidade atual medida é:","");
  client.publish(umi,"%");
  delay (5000);
}

//Função de repetição (será executado INFINITAMENTE até o ESP ser desligado)
void loop() {
  if (!client.connected()) {
    reconectar();
  }
  tem_umi();
  timerWrite(timer, 0); //reseta o temporizador (alimenta o watchdog)
}
