#include <M5StickC.h>

#include <WiFi.h>
#include <WiFiUdp.h>

#define max_sine 0.4
#define pool_time 200
#define takeoff_time 3000
#define land_time 10000

// const char *ssid = "Uma vez flamengo";
// const char *password = "S3MPR3FL4M3NG0304";

// const char* TELLO_IP = "192.168.0.19";
// const int PORT = 3000;

const char *ssid = "TELLO-C470F1";
const char *password = "";

const char* TELLO_IP = "192.168.10.1";
const int PORT = 8889;

WiFiUDP Udp;
char packetBuffer[255];
String message = "";

float accX = 0.0F;
float accY = 0.0F;
float accZ = 0.0F;

bool isOnAir = false;
bool conectado = false;

int movementState = 0;
int readState = 0;
unsigned long msg_time;

void printMsg(int x, int y, int w, int h, uint16_t color, String msg) {
  M5.Lcd.fillRect(x, y, w, h, color);
  M5.Lcd.setCursor(x, y+3);
  M5.Lcd.println(msg);
}

void print_status(String status_msg, uint16_t color){
  printMsg(0, 14, 80, 15, BLACK, status_msg);
}

void print_movement_state(){
  if (movementState == 0) {
    print_status("Horizontal", BLUE);
  } else if (movementState == 1) {
    print_status("Vertical", CYAN);
  } else {
    print_status("Steady", NAVY);
  }
}

void print_air_state(){
  printMsg(0, 0, 80, 15, isOnAir ? RED : GREEN, isOnAir ? "On Air" : "On Land");
}

void print_command(String status_msg){
  printMsg(0, 79, 80, 40, BLACK, status_msg);
}

void print_message(String cmd){
  printMsg(0, 139, 80, 40, PINK, cmd + "\n> " + message);
}

void tello_command_exec(char* tello_command){
  print_command(tello_command);

  Udp.beginPacket(TELLO_IP, PORT);
  Udp.printf(tello_command);
  Udp.endPacket();
  listenMessage(tello_command);
  delay(100);
  msg_time = millis();
}

void listenMessage(String cmd) {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    IPAddress remoteIp = Udp.remoteIP();
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
    }
  }
  message = (char*) packetBuffer;
  print_message(cmd);
}

void ConectarWifi() {
  print_status(" Conectando", YELLOW);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Lcd.print(".");
  }
  M5.Lcd.fillScreen(BLACK);

  print_status(" WiFi OK", GREEN);
  delay(1000);
}

void setup() {
  M5.begin();
  M5.IMU.Init();
  Wire.begin();

  ConectarWifi();
  
  Udp.begin(PORT);
  tello_command_exec("command");
  delay(2000);
  tello_command_exec("speed 40");
}

void loop() {
  print_air_state();
  M5.IMU.getAccelData(&accX, &accY, &accZ);

  // land / take off
  if (M5.BtnB.wasPressed()) {
    if (isOnAir) {
      tello_command_exec("land");
      delay(land_time);
    } else {
      tello_command_exec("takeoff");
      delay(takeoff_time);
    }
    isOnAir = !isOnAir;
    print_air_state();
  }
  
  // change state
  if (M5.BtnA.wasPressed()) {
    movementState += 1;
    if (movementState == 3) movementState = 0; 
  }
  print_movement_state();

  if (isOnAir) {
    if (movementState == 0) {
      if (accY > max_sine) {
        tello_command_exec("back 50"); // down
      } else if (accY < -max_sine) {
        tello_command_exec("forward 50"); // up
      }
      
      if (accX > max_sine) {
        tello_command_exec("left 50"); // left
      } else if (accX < -max_sine){
        tello_command_exec("right 50"); // right
      }
    } else if (movementState == 1) {
      if (accY > max_sine) {
        tello_command_exec("up 50");
      } else if (accY < -max_sine) {
        tello_command_exec("down 50");
      }
      
      if (accX > max_sine) {
        tello_command_exec("ccw 30");
      } else if (accX < -max_sine){
        tello_command_exec("cw 30");
      }
    }
  } 

  if (millis() - msg_time > 3000) { // keep-alive
    if (readState == 0) {
      tello_command_exec("battery?");
      readState = 1;
    } else if (readState == 1) {
      tello_command_exec("wifi?");
      readState = 2;
    } else if (readState == 2) {
      tello_command_exec("time?");
      readState = 3;
    } else {
      tello_command_exec("temp?");
      readState = 0;
    }
  }

  delay(pool_time);
  M5.update();
}
