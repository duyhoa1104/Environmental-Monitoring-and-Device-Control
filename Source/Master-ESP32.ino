// Include Library.
#include <SPI.h>                     // Thư viện SPI
#include <LoRa.h>                    // Thư viện LoRa
#include <Wire.h>                    // Thư viện I2C
#include <Adafruit_GFX.h>            // Thư viện cho OLED
#include <Adafruit_SSD1306.h>        // Thư viện cho OLED
#include <FirebaseESP32.h>           // Thư viện Firebase cho ESP32
#include <WiFi.h>                    // Thư viện Wifi

// Khai báo chân sử dụng cho các nút nhấn.
#define Button_1_Pin 13
#define Button_2_Pin 12

// Cấu hình kích thước màn hình OLED theo pixels.
#define SCREEN_WIDTH 128    // độ rộng OLED, in pixels
#define SCREEN_HEIGHT 64    // độ cao OLED, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins).
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define FIREBASE_HOST "https://esp32-lora-dht11-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "K1GpC3vvYwVzkzJtpu8PmFIrVOFldA1cuCDfmI2V"

#define WIFI_SSID "Viettel_01"          // Tên Wifi kết nối với ESP32  
#define WIFI_PASSWORD "4761959701"     // Password Wifi

// LoRa Pin / GPIO configuration.
#define ss 5
#define rst 14
#define dio0 2

FirebaseData fbdo;    // biến trung gian để giao tiếp với Firebase  

//  Variable declaration to hold incoming and outgoing data.
String Incoming = "";
String Message = "";             

// LoRa data transmission configuration.
byte LocalAddress = 0x01;               //--> Master Address.
byte Destination_ESP32_Slave_1 = 0x02;  //--> destination to send to Slave 1 (ESP32).

// Variable declaration for Millis/Timer.
unsigned long previousMillis_SendMSG = 0;
unsigned long previousMillis_RestartLORA = 0;

// Declaration of helper variables for the operation of the buttons.
bool flag_led_1 = 0;
bool flag_led_2 = 0;

int LED_1_State = 0;
int LED_2_State = 0;

float Humd;
float Temp;
int Rssi;
int slv = 1;

String LEDs_State = "";         // Variable declaration to get LED state on slaves.
byte Count_RS_LORA = 0;     // Declaration of variable as counter to restart Lora Ra-02.

// Chương trình con thiết lập và gửi gói tin.
void sendMessage(String Outgoing, byte Destination) {
  LoRa.beginPacket();             //--> start packet
  LoRa.write(Destination);        //--> add destination address 
  LoRa.write(LocalAddress);       //--> add sender address
  LoRa.write(Outgoing.length());  //--> add payload length
  LoRa.print(Outgoing);           //--> add payload
  LoRa.endPacket();               //--> finish packet and send it
}

// Chương trình con nhận gói tin và các tác vụ xử lý.
void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // Read packet header bytes:
  int recipient = LoRa.read();        // recipient address
  byte sender = LoRa.read();          // sender address
  byte incomingLength = LoRa.read();  // incoming msg length
  //---------------------------------------- 

  Incoming = "";   // Xóa dữ liệu của biến chứa nội dung gói tin cũ

  // Lưu dữ liệu gói tin mới đến vào biến Incoming.
  while (LoRa.available()) {
    Incoming += (char)LoRa.read();
  }
  //---------------------------------------- 

  Count_RS_LORA = 0;

  // Kiểm tra độ dài của gói tin đến.
  if (incomingLength != Incoming.length()) {
    Serial.println();
    Serial.println("Error: message length does not match length");
    return;
  }
  //---------------------------------------- 

  // kiểm tra địa chỉ của bên gửi gói tin
  if (sender != Destination_ESP32_Slave_1) {
    Serial.println();
    Serial.println("Not from Slave, Ignore");
    return;
  }

  // Kiểm tra xem gói tin có gửi đến đúng địa chỉ cho thiết bị này.
  if (recipient != LocalAddress) {
    Serial.println();
    Serial.println("This message is not for me.");
    return;
  }
  //-------------------------------
  
  // Xuất ra Serial nếu nhận được tin nhắn
  Rssi = LoRa.packetRssi();
  Serial.println();
  Serial.println("Received from slave 1");
  Serial.print("Message: '" + Incoming);
  Serial.print("' with RSSI ");
  Serial.println(Rssi);
  //-------------------------------------

  // Gọi đến chương trình con xử lý dữ liệu gói tin.
  Processing_incoming_data();
}

// Chương trình con xử lý dữ liệu gói tin nhận được.
void Processing_incoming_data() {
  Humd = GetValue(Incoming, ',', 0).toFloat();
  Temp = GetValue(Incoming, ',', 1).toFloat();
  Update_OLED();
}
//-------------------------------------------------

// Chương trình dùng hiển thị dữ liệu gói tin của slave lên OLED.
void Update_OLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Slave 1 (ESP32)");
  display.setCursor(0, 10);
  display.print("Nhiet do = ");
  display.print(Temp);
  display.print(" ");
  display.print((char)247); //--> ASCII degree symbol
  display.print("C");
  display.setCursor(0, 20);
  display.print("Do am = ");
  display.print(Humd);
  display.print(" %");
  display.setCursor(0, 30);
  display.print("LED_1 = ");
  if (LED_1_State == 1) display.print("ON");
  if (LED_1_State == 0) display.print("OFF");
  display.setCursor(0, 40);
  display.print("LED_2 = ");
  if (LED_2_State == 1) display.print("ON");
  if (LED_2_State == 0) display.print("OFF");
  display.setCursor(0, 50);
  display.print("RSSI = ");
  display.print(Rssi);
  display.print(" dBm");
  display.display();
}

// Hàm chuỗi để xử lý dữ liệu được truyền vào.
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
//----------------------------------------

// Chương trình con dùng cập nhật dữ liệu trên Firebase.
void Update_Firebase() {
  Firebase.getInt(fbdo, "/Kho01/led01");
  LED_1_State = fbdo.intData();
  Firebase.getInt(fbdo, "/Kho01/led02");
  LED_2_State = fbdo.intData();
 
  Firebase.setFloat(fbdo,"/Kho01/Humi", Humd);
  Firebase.setFloat (fbdo,"/Kho01/Temp", Temp);
  Firebase.setFloat (fbdo,"/Kho01/RSSI", Rssi);
}

// Chương trình con dùng lấy dữ liệu cho lần đầu tiên.
void Get_data_first_time() {
  Serial.println();
  Serial.println("Getting data for the first time...");
  delay(500);
  Serial.println("Getting data for the first time has been completed.");
    
  Message = "N,N";

  Serial.print("Send message to Slave " + String(slv));
  Serial.println(" for first time: " + Message);
  sendMessage(Message, Destination_ESP32_Slave_1);  

  onReceive(LoRa.parsePacket());
}

// Chương trình con dùng reset module Lora
void Reset_LORA() {
  LoRa.setPins(ss, rst, dio0);

  Serial.println();
  Serial.println("Restart LoRa...");
  Serial.println("Start LoRa init...");
  if (!LoRa.begin(433E6)) {             // Cài đặt tần số lora   433 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }
  Serial.println("LoRa init succeeded.");

  Count_RS_LORA = 0;
}

// Chương trình con kiểm tra trạng thái nút nhấn
void Check_button(){
  if(flag_led_1 == 1){
    LED_1_State = !LED_1_State;
    Serial.print("led_01: " );
    Serial.println(LED_1_State);
    Firebase.setInt(fbdo, "/Kho01/led01", LED_1_State);
    flag_led_1 = 0;
  }
  if(flag_led_2 == 1){
    LED_2_State = !LED_2_State;
    Serial.print("led_02: " );
    Serial.println(LED_2_State);
    Firebase.setInt(fbdo, "/Kho01/led02", LED_2_State);
    flag_led_2 = 0;
  }
}

//_______________ VOID SETUP
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Khai báo sử dụng ngắt ngoài trên chân Button_1_pin kết hợp kéo trở có sẵn
  pinMode(Button_1_Pin, INPUT_PULLUP);
  pinMode(Button_2_Pin, INPUT_PULLUP);

  // Hàm Interrupt gọi đến chương trình con INT_Button_1 khi chân Button_1_pin bắt gặp tín hiệu cạnh xuống (từ 1 xuống 0)
  attachInterrupt(Button_1_Pin, INT_Button_1, FALLING);
  attachInterrupt(Button_2_Pin, INT_Button_2, FALLING);
  delay(500);

  // Khởi tạo Wifi và kiểm tra kết nối
  WiFi.begin (WIFI_SSID, WIFI_PASSWORD);
  Serial.println();
  Serial.print("Dang ket noi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println ("");
  Serial.println ("Da ket noi WiFi!");
  Serial.println(WiFi.localIP());

  // Khởi tạo Firebase với địa chỉ HOST và AUTH
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  // SSD1306_SWITCHCAPVCC = Tạo điện áp hiển thị từ 3V3 bên trong.
  // Address 0x3C for 128x32 and Address 0x3D for 128x64.
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); //--> Don't proceed, loop forever
  }
  //----------------------------------------

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(38, 15);
  display.println("WELCOME");
  display.setCursor(30, 35);
  display.println("ESP32 LORA");
  display.display();
  delay(2000);

  // Xóa giá trị của biến nhiệt độ & độ ẩm cho lần đầu tiên.
  Humd = 0.0;
  Temp = 0.0;

  Update_OLED();

  Reset_LORA();

  Get_data_first_time();
}
//_______________________________________________________ 
//____________________ VOID LOOP
void loop() {
  // put your main code here, to run repeatedly:
  
  // Cập nhật dữ liệu Firebase.
  Update_Firebase();

  // Kiểm tra trạng thái nút nhấn.
  Check_button();
  
  // Chương trình con thiết lập dữ liệu và gửi gói tin đến Slave.
  if (millis() - previousMillis_SendMSG >= 3000) {
    previousMillis_SendMSG = millis();

    String send_LED_1_State = ""; 
    String send_LED_2_State = "";
    if (LED_1_State == 1) send_LED_1_State = "t";
    else send_LED_1_State = "f";
    if (LED_2_State == 1) send_LED_2_State = "t";
    else send_LED_2_State = "f";

    Message = send_LED_1_State + "," + send_LED_2_State;      // Dữ liệu trong gói tin được gửi.
    Serial.println();
    Serial.print("Send message to Slave " + String(slv));
    Serial.println(": " + Message);
    sendMessage(Message, Destination_ESP32_Slave_1);
  }
 
  // Chương trình con nhận gói tin đến.
  onReceive(LoRa.parsePacket());

  // Chương trình con dùng Reset Lora.
  if (millis() - previousMillis_RestartLORA >= 1000) {
    previousMillis_RestartLORA = millis();

    Count_RS_LORA++;
    if (Count_RS_LORA > 30) {
      LoRa.end();
      Reset_LORA();
    }
  }

}
//__________________________________________________________

// Hàm xử lý ngắt nút nhấn
void INT_Button_1(){
  flag_led_1 = 1;
}
void INT_Button_2(){
  flag_led_2 = 1;
}
