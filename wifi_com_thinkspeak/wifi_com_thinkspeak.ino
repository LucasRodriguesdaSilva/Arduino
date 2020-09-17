// Thingspeak  
String canalID1 = "XXXXXXX"; // ID DO CANAL NO THINGSPEAK

#include <SoftwareSerial.h>
SoftwareSerial EspSerial(10, 11); //PINOS NO ESP8266 Rx,  Tx
#define HARDWARE_RESET 7 

// Variables to be used with timers
long readTimingSeconds = 10; // ==> Define Sample time in seconds to receive data
long startReadTiming = 0;
long elapsedReadTime = 0;

//Relays
#define ACTUATOR1 2 // PINO DO RELE PARA ACIONAR LED   ==> Pump
boolean pump = 0; 

int spare = 0;
boolean error;

void setup()
{
  Serial.begin(9600);
  
  pinMode(ACTUATOR1,OUTPUT);
  pinMode(HARDWARE_RESET,OUTPUT);
  digitalWrite(ACTUATOR1, LOW); //o módulo relé é ativo em LOW
  digitalWrite(HARDWARE_RESET, HIGH);
  EspHardwareReset(); //Reset do Modulo WiFi
  EspSerial.begin(9600); // Comunicacao com Modulo WiFi
  startReadTiming = millis(); // starting the "program clock"
}

void loop()
{
  start: //label 
  error=0;
  
  elapsedReadTime = millis()-startReadTiming; 

  if (elapsedReadTime > (readTimingSeconds*1000)) 
  {
    int command = readThingSpeak(canalID1); 
    if (command != 9) pump = command; 
    delay (5000);
    takeActions();
    startReadTiming = millis();   
  }
  
  if (error==1) //Resend if transmission is not completed 
  {       
    Serial.println(" <<<< ERROR >>>>");
    delay (2000);  
    goto start; //go to label "start"
  }
}

/********* Take actions based on ThingSpeak Commands *************/
void takeActions(void)
{
  Serial.print("Pump: ");
  Serial.println(pump);
  if (pump == 1) digitalWrite(ACTUATOR1, HIGH);
  else digitalWrite(ACTUATOR1, LOW);
}

/********* Read Actuators command from ThingSpeak *************/
int readThingSpeak(String channelID)
{
  startThingSpeakCmd();
  int command;
  // preparacao da string GET
  String getStr = "GET /channels/";
  getStr += channelID;
  getStr +="/fields/1/last";
  getStr += "\r\n";

  String messageDown = sendThingSpeakGetCmd(getStr);
  if (messageDown[5] == 49)
  {
    command = messageDown[7]-48; 
    Serial.print("Command received: ");
    Serial.println(command);
  }
  else command = 9;
  return command;
}
/********* Reset ESP *************/
void EspHardwareReset(void)
{
  Serial.println("Reseting......."); 
  digitalWrite(HARDWARE_RESET, LOW); 
  delay(500);
  digitalWrite(HARDWARE_RESET, HIGH);
  delay(8000);//Tempo necessário para começar a ler 
  Serial.println("RESET"); 
}
/********* Start communication with ThingSpeak*************/
void startThingSpeakCmd(void)
{
  EspSerial.flush();//limpa o buffer antes de começar a gravar
  
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "184.106.153.149"; // Endereco IP de api.thingspeak.com
  cmd += "\",80";
  EspSerial.println(cmd);
  Serial.print("enviado ==> Start cmd: ");
  Serial.println(cmd);

  if(EspSerial.find("Error"))
  {
    Serial.println("AT+CIPSTART error");
    return;
  }
}

/********* send a GET cmd to ThingSpeak *************/
String sendThingSpeakGetCmd(String getStr)
{
  String cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  EspSerial.println(cmd);
  Serial.print("enviado ==> lenght cmd: ");
  Serial.println(cmd);

  if(EspSerial.find((char *)">"))
  {
    EspSerial.print(getStr);
    Serial.print("enviado ==> getStr: ");
    Serial.println(getStr);
    delay(500);//tempo para processar o GET, sem este delay apresenta busy no próximo comando

    String messageBody = "";
    while (EspSerial.available()) 
    {
      String line = EspSerial.readStringUntil('\n');
      if (line.length() == 1) 
      { //actual content starts after empty line (that has length 1)
        messageBody = EspSerial.readStringUntil('\n');
      }
    }
    Serial.print("MessageBody received: ");
    Serial.println(messageBody);
    return messageBody;
  }
  else
  {
    EspSerial.println("AT+CIPCLOSE");     // alert user
    Serial.println("ESP8266 CIPSEND ERROR: RESENDING"); //Resend...
    spare = spare + 1;
    error=1;
    return "error";
  } 
}
