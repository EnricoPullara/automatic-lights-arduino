#include <SoftwareSerial.h>
#include <EEPROM.h>
#include "config.h"

SoftwareSerial esp8266(RX_ETH,TX_ETH);

void setup(){
    
  Serial.begin(250000);
  esp8266.begin(9600); // esp baud rate

  Serial.println("Start Setup");

  pinMode(RES_ETH, OUTPUT);
  digitalWrite (RES_ETH, HIGH);

  for (byte i = 2; i < 11; i++){
    pinMode(i, OUTPUT);
  }
  init_esp8266();
  Serial.println("Connection Estabilished, starting...");
  
}

void loop() {
  if (readServer()) {
    Serial.println("Loop OK");
  } else {
    Serial.println("Connection Problem");
    init_esp8266();
  }
  delay(1000);
}

void init_esp8266(){
  //Reset esp
  while (!sendData("AT+RST\r\n", 3000, "ready")){
    Serial.println("Not Ready, sending again");
  }
 
  delay(1000);
  //Network connection
  while (!sendData("AT+CWJAP=\"" + String(NETWORK_SSID) + "\",\"" + String(NETWORK_PSW) + "\"\r\n", 12000, "OK")) {
    Serial.println("Can't connect, trying again");
  }
  sendData("AT+CWMODE=1\r\n", 2000, "no change");///Station Mode
  sendData("AT+CIPMUX=0\r\n", 2000, "OK"); //Multiplex disattivato
}

bool sendData(String command, const int timeout, String expRes) {
  String incomingData = "";
  esp8266.flush();
  esp8266.print(command);
  long int time = millis();
  while ( (time + timeout) > millis()) {
    incomingData += readEsp();
    if (incomingData.indexOf(expRes) > 0 || incomingData.indexOf("ALREAY CONNECT") > 0) {
      return true;
    }
  }
  Serial.println("****Send Data failed");
  Serial.println("Command: " + command);
  Serial.println("Result: '\n" + incomingData + "\n'");
  return false;
}

boolean readServer(){
  String incomingData = "";
  if (!sendData("AT+CIPSTART=\"TCP\",\""+ String(HOST_REM) + "\",80\r\n",2000,"OK")) {
    return false;
  }
  String path = String(FILE_REM) + "?rand=" + rand();
  String getRequest = "GET " + path + " HTTP/1.1\r\nHost: " + String(HOST_REM) + "\r\n\r\n";
  if(!sendData("AT+CIPSEND=" + String(getRequest.length()) + "\r\n",5000,">")) {
    return false;
  }
  
  Serial.println("\nFine CIPSEND, Sending GET");
  
  esp8266.print(getRequest);
  long int time = millis();
  while ( (time + 5000) > millis()) {
    incomingData += readEsp();
  }
  esp8266.flush();
  int startIndex = incomingData.indexOf("ChX");
  int endIndex = incomingData.indexOf("EOF");
  if (startIndex < 0 || endIndex < 0) return false;
  Serial.println("Result GET: "+ incomingData);
  //Serial.println("Index: " + String(startIndex));
  //Serial.println("Index: " + String(endIndex));
  for (int i = 0; (i + startIndex + 3) < endIndex; i++) {
    digitalWrite(i, (char(incomingData[(i + startIndex + 3)]) == '1'));
    Serial.println(i);
    Serial.println(incomingData[(i + startIndex + 3)]);
  }
  return true;
}



String readEsp() {
  String _res = "";
  while (esp8266.available()) {
    char c = esp8266.read();
    _res += c;
  }
  return _res;
}
