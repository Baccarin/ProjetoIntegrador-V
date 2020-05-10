#include <WiFi.h>        
#include <PubSubClient.h>

//Dados do seu WIFI
const char *ssid = "Nome_da_rede_wifi";
const char *password = "senha_wifi";

#define DEBUG                   //Se comentar para a impressão na porta serial

// DADOSDO MQTT
#define servidor_mqtt             "m24.cloudmqtt.com"  //URL do servidor MQTT
#define servidor_mqtt_porta       "porta"  //Porta do servidor
#define servidor_mqtt_usuario     "usuário_mqtt"  //Usuário
#define servidor_mqtt_senha       "senha_mqtt"  //Senha
#define mqtt_topico_sub           "REDE"    //Tópico para SUBSCREVER 
#define mqtt_topico_pub           "REDE"    //Tópico para PUBLICAR


#define led             02               //pino led  
#define botao           12               //pino botao publica quando esta em nivel alto

WiFiClient espClient;                                 //Instância do WiFiClient
PubSubClient client(espClient);                       //Passando a instância do WiFiClient para a instância do PubSubClient


//Função para imprimir na porta serial
void imprimirSerial(bool linha, String mensagem){
  #ifdef DEBUG
    if(linha){
      Serial.println(mensagem);
    }else{
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

    if(conectado) {
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





//Função que será chamada ao receber mensagem do servidor MQTT
void retorno(char* topico, byte* mensagem, unsigned int tamanho) {
  //Convertendo a mensagem recebida para string
  mensagem[tamanho] = '\0';
  String strMensagem = String((char*)mensagem);
  strMensagem.toLowerCase();
  
  imprimirSerial(false, "Mensagem recebida! Topico: ");
  imprimirSerial(false, topico);
  imprimirSerial(false, ". Tamanho: ");
  imprimirSerial(false, String(tamanho).c_str());
  imprimirSerial(false, ". Mensagem: ");
  imprimirSerial(true, strMensagem);

  //Executando o comando solicitado
  imprimirSerial(false, "Status do pino antes de processar o comando: ");
  imprimirSerial(true, String(digitalRead(led)).c_str());

// Aqui liga e desliga o pino
  if(strMensagem == "ligar"){
    digitalWrite(led, HIGH);
    client.publish(mqtt_topico_pub, "O led foi ligado" );
    delay(2000);
    client.publish(mqtt_topico_pub, "Digite desligar para desligar o led" );
  }else if(strMensagem == "desligar"){
   client.publish(mqtt_topico_pub, "O led foi desligado" );
   digitalWrite(led, LOW);
    delay(2000);
    client.publish(mqtt_topico_pub, "Digite ligar para ligar o led" );
  }
  
  imprimirSerial(false, "Status do pino depois de processar o comando: ");
  imprimirSerial(true, String(digitalRead(led)).c_str());
}

//Função inicial (será executado SOMENTE quando ligar o ESP)
void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
  #endif
  imprimirSerial(true, "...");

//inicializa WiFi
  setupWiFi();
  
  //Fazendo o pino ser de saída, pois ele irá "controlar" algo.
  pinMode(led, OUTPUT);
  //Fazendo o pino ser de entrada
  pinMode(botao, OUTPUT);
  
  imprimirSerial(false, "IP: ");
  imprimirSerial(true, WiFi.localIP().toString());

  //Informando ao client do PubSub a url do servidor e a porta.
  int portaInt = atoi(servidor_mqtt_porta);
  client.setServer(servidor_mqtt, portaInt);
  client.setCallback(retorno);
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

//Função de repetição (será executado INFINITAMENTE até o ESP ser desligado)
void loop() {
  if (!client.connected()) {
    reconectar();
  }
  client.loop();
  
  // Se descomentar Publica mensagem no topico
  if (((digitalRead(botao) == HIGH))){
  delay(1000);
  client.publish(mqtt_topico_pub, "Redes tudo certo" );
  }
}
