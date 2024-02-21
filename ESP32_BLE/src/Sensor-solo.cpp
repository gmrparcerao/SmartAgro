// Bibliotecas para o ESP32, WiFi e protocolo ESPNOW
#include <Arduino.h>
#include <WiFi.h>
#include "ESPNowW.h"

//Endereço MAC do receptor
uint8_t receiver_mac[] = {0xC8, 0xC9, 0xA3, 0xC7, 0xED, 0x68};
//uint8_t receiver_mac[] = {0xAC, 0x67, 0xB2, 0x2C, 0xA7, 0xC8};(prof Caio)
hw_timer_t *timer = NULL; // Variável que indica a posição de memória do timer

// variáveis do programa
const int pinoSensor = 35;
const int pinoValvula = 14;
//const int limiarSeco = 0;
const int tempoRega = 50; // Tempo de rega em segundos
int umidadeSolo = 0;
int sensorValue = 0;
float Valor = 0.00;

void IRAM_ATTR onTimer() {
  sensorValue = analogRead(pinoSensor);
  Valor =  3.3/4095 * sensorValue; //Alimentação de 3.3 Volts 
  Serial.print(Valor,2);//Valores em Volt com 2 casas decimais
  Serial.println("V");
  Serial.print(umidadeSolo);
  Serial.println("%");
}

// Estrutura da mensagem que será recebida
typedef struct struct_message{

  float U = Valor;
  // string
  //char a[32] = "Volts";

} struct_message;

struct_message myData;

void initESPNow(){

  WiFi.disconnect();
  ESPNow.init();
  ESPNow.add_peer(receiver_mac);

}

void setup(){

  Serial.begin(9600);

  WiFi.disconnect(); // precisei desconectar para ter receber os dados dia ESP-NOW
  delay(1000);
  WiFi.mode(WIFI_MODE_STA);
  delay(1000);

  initESPNow(); // apenas separei em outra função...
  
  pinMode(pinoValvula, OUTPUT);
  // Desliga a válvula
  digitalWrite(pinoValvula, LOW); //HIGH
  
//  Serial.println("Initializing Acquisition...");
  timer = timerBegin(0, 80, true);  // Clock do MCU / 80 = 1MHz
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 10000, true); // 1MHz/10000 = 100Hz
  timerAlarmEnable(timer);
}

void loop(){
  // Mede a umidade a cada segundo. Faz isso durante uma hora (3600 segundos).
  for(int i=0; i < 5; i++) {
  
     // Faz a leitura do sensor de umidade do solo
    umidadeSolo = analogRead(pinoSensor);
    // Converte a variação do sensor de 0 a 1023 para 0 a 100
    umidadeSolo = map(umidadeSolo, 4095, 0, 0, 100);
 
    // Espera um segundo
    delay(1000);
  }
  
  if(umidadeSolo < 19) {
 
    // Liga a válvula
    digitalWrite(pinoValvula, HIGH); //LOW
    // Espera o tempo estipulado
//    delay(tempoRega*1000);
//    delay(tempoRega*100); // remover e testar, não terá tempo de rega
//    digitalWrite(pinoValvula, LOW);//HIGH
  }

  else if (umidadeSolo > 35) //Ou sensor de chuva true
  { 

//else if ... verificar pressão umidade, em caso da possibilidade de prescipitação de chuva ...aguardar (...delay ou) até umidade do solo crítica 70%

//    desliga a válvula
    digitalWrite(pinoValvula, LOW);//HIGH
  }

  else{
    // Espera o tempo estipulado
    delay(3000);
 }
  ESPNow.send_message(receiver_mac, (uint8_t*)&myData, sizeof(myData));

}   
