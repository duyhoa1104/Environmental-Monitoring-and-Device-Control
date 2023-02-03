//------- Include Library.
#include <SPI.h>
#include <LoRa.h>
#include "DHT.h"
//---------------------------------------- 

//------- Defines the DHT11 Pin and the DHT type.
#define DHTPIN      32
#define DHTTYPE     DHT11
//---------------------------------------- 

//------- Defines LED Pins.
#define LED_1_Pin   27
#define LED_2_Pin   26
//---------------------------------------- 

//------- LoRa Pin / GPIO configuration.
#define ss 5
#define rst 14
#define dio0 2
//----------------------------------------

// Initializes the DHT sensor (DHT11).
DHT dht11(DHTPIN, DHTTYPE);

String Incoming = "";
String Message = "";

String CMD_LED_1_State = "";
String CMD_LED_2_State = "";

byte LocalAddress = 0x02;       //--> address of this device (Slave 1).
byte Destination_Master = 0x01; //--> destination to send to Master (ESP32).

float h;
float t;
int Rssi;

unsigned long previousMillis_UpdateDHT11 = 0;
const long interval_UpdateDHT11 = 2000;

unsigned long previousMillis_RestartLORA = 0;
const long interval_RestartLORA = 1000;

byte Count_to_Rst_LORA = 0;

//  Chương trình con thiết lập và gửi gói tin.
void sendMessage(String Outgoing, byte Destination) {
  LoRa.beginPacket();             //--> start packet
  LoRa.write(Destination);        //--> add destination address
  LoRa.write(LocalAddress);       //--> add sender address
  LoRa.write(Outgoing.length());  //--> add payload length
  LoRa.print(Outgoing);           //--> add payload
  LoRa.endPacket();               //--> finish packet and send it
}
//________________________________________________________________________________ 

// Chương trình con nhận gói tin và các tác vụ xử lý
void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  //---------------------------------------- read packet header bytes:
  int recipient = LoRa.read();        //--> recipient address
  byte sender = LoRa.read();          //--> sender address
  byte incomingLength = LoRa.read();  //--> incoming msg length
  //---------------------------------------- 

  // Kiểm tra địa chỉ của bên gửi gói tin
  if (sender != Destination_Master) {
    Serial.println();
    Serial.println("Not from Master, Ignore");

    // Resets the value of the Count_to_Rst_LORA variable.
    Count_to_Rst_LORA = 0;
    return;
  }

  Incoming = "";

  // Lưu dữ liệu gói tin mới đến vào biến Incoming.
  while (LoRa.available()) {
    Incoming += (char)LoRa.read();
  }

  Count_to_Rst_LORA = 0;

  // Kiểm tra độ dài của gói tin đến.
  if (incomingLength != Incoming.length()) {
    Serial.println();
    Serial.println("Error: message length does not match length.");
    return;
  }
  //---------------------------------------- 

  // Kiểm tra xem gói tin có gửi đến đúng địa chỉ cho thiết bị này
  if (recipient != LocalAddress) {
    Serial.println();
    Serial.println("This message is not for me.");
    return;
  } else {
    Rssi = LoRa.packetRssi();
    Serial.println();
    Serial.println("Received from Master");
    Serial.print("Message: '" + Incoming);
    Serial.print("' with RSSI ");
    Serial.println(Rssi);

    Processing_incoming_data();
  }
}

// Chương trình con xử lý dữ liệu gói tin nhận được
void Processing_incoming_data() {

  Processing_incoming_data_for_LEDs();

  // Get the last state of the LED.
  byte LED_1_State = digitalRead(LED_1_Pin);
  byte LED_2_State = digitalRead(LED_2_Pin);

  // Fill in the "Message" variable with the value of humidity, temperature and the last state of the LED.
  Message = String(h) + "," + String(t) + "," + String(LED_1_State) + "," + String(LED_2_State);
  
  Serial.println();
  Serial.print("Send to Master ");
  Serial.println("message: " + Message);
  // Send a message to Master.
  sendMessage(Message, Destination_Master);
}

// Chương trình con xử lý đèn led
void Processing_incoming_data_for_LEDs() {

  CMD_LED_1_State = GetValue(Incoming, ',', 0);
  CMD_LED_2_State = GetValue(Incoming, ',', 1);

  if (CMD_LED_1_State == "t") digitalWrite(LED_1_Pin, HIGH);    // t: bật  &  f: tắt
  if (CMD_LED_1_State == "f") digitalWrite(LED_1_Pin, LOW);

  if (CMD_LED_2_State == "t") digitalWrite(LED_2_Pin, HIGH);
  if (CMD_LED_2_State == "f") digitalWrite(LED_2_Pin, LOW);
}

// Hàm chuỗi dùng xử lý dữ liệu truyền vào
String GetValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
//________________________________________________________________________________ 

// Chương trình con Reset LoRa
void Rst_LORA() {
  LoRa.setPins(ss, rst, dio0);

  Serial.println();
  Serial.println(F("Restart LoRa..."));
  Serial.println(F("Start LoRa init..."));
  if (!LoRa.begin(433E6)) {             // initialize ratio at 915 or 433 MHz
    Serial.println(F("LoRa init failed. Check your connections."));
    while (true);                       // if failed, do nothing
  }
  Serial.println(F("LoRa init succeeded."));

  // biến dùng để Reset LoRa
  Count_to_Rst_LORA = 0;
}
//________________________________________________________________________________ 

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  pinMode(LED_1_Pin, OUTPUT);
  pinMode(LED_2_Pin, OUTPUT);

  Rst_LORA();
  dht11.begin();
}
//________________________________________________________________________________ 

void loop() {
  // put your main code here, to run repeatedly:

  // Chương trình đọc và gán thông số nhiệt độ & độ ẩm sau mỗi 2s
  unsigned long currentMillis_UpdateDHT11 = millis(); 
  if (currentMillis_UpdateDHT11 - previousMillis_UpdateDHT11 >= interval_UpdateDHT11) {
    previousMillis_UpdateDHT11 = currentMillis_UpdateDHT11;

    h = dht11.readHumidity();
    t = dht11.readTemperature();

    // Kiểm tra cảm biến có đọc được thông số không
    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }
  }
  
  // Chương trình Reset LoRa
  unsigned long currentMillis_RestartLORA = millis();
  if (currentMillis_RestartLORA - previousMillis_RestartLORA >= interval_RestartLORA) {
    previousMillis_RestartLORA = currentMillis_RestartLORA;

    Count_to_Rst_LORA++;
    if (Count_to_Rst_LORA > 30) {
      LoRa.end();
      Rst_LORA();
    }
  }

  // Chương trình con nhận gói tin đến
  onReceive(LoRa.parsePacket());
}
