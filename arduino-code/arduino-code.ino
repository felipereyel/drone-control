#include <ArduinoWebsockets.h>
#include <M5StickC.h>
#include <WiFi.h>

#define max_sine 0.4
#define min_mov 20
#define pool_time 500
#define takeoff_time 3000
#define land_time 10000

const char *ssid = "Uma vez flamengo";      // Enter SSID
const char *password = "S3MPR3FL4M3NG0304"; // Enter Password
const char *websockets_server =
    "ws://192.168.0.19:8000"; // server adress and port

using namespace websockets;

float accX = 0.0F;
float accY = 0.0F;
float accZ = 0.0F;

int movementState = 0;
bool isOnAir = false;
bool conectado = false;

WebsocketsClient client;

void initScreen() {
  M5.Lcd.fillTriangle(40, 159, 60, 139, 20, 139, BLACK);
  M5.Lcd.drawTriangle(40, 159, 60, 139, 20, 139, RED);

  M5.Lcd.fillTriangle(40, 0, 60, 20, 20, 20, BLACK);
  M5.Lcd.drawTriangle(40, 0, 60, 20, 20, 20, BLUE);

  M5.Lcd.fillTriangle(0, 80, 20, 100, 20, 60, BLACK);
  M5.Lcd.drawTriangle(0, 80, 20, 100, 20, 60, GREEN);

  M5.Lcd.fillTriangle(79, 80, 59, 100, 59, 60, BLACK);
  M5.Lcd.drawTriangle(79, 80, 59, 100, 59, 60, YELLOW);
}

void onMessageCallback(WebsocketsMessage message) {
  Serial.println(message.c_str());
}

void onEventsCallback(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    conectado = true;
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    delay(1000);
    ESP.restart();
  } else if (event == WebsocketsEvent::GotPing) {
    Serial.println("Got a Ping!");
  } else if (event == WebsocketsEvent::GotPong) {
    Serial.println("Got a Pong!");
  }
}

void ConectarWifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.println("Conectando");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    M5.Lcd.print(".");
  }

  Serial.println("WiFi conectado!");
  Serial.println("Endereco de IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.println("Conectado");
  delay(1000);
  M5.Lcd.fillScreen(BLACK);
}

void ConectarWS() {
  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);

  Serial.print("Connecting to WS");
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.println("Conectando WS");
  
  while (!conectado) {
    client.connect(websockets_server);
    Serial.print(".");
    M5.Lcd.print(".");
    delay(1000);
  }

  Serial.println("WS conectado!");
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.println("WS Conectado");
  delay(1000);
  M5.Lcd.fillScreen(BLACK);

  client.send("initialize / command");
}

void setup() {
  M5.begin();
  Serial.begin(115200);

  M5.IMU.Init();
  M5.Lcd.setRotation(0);
  M5.Lcd.setTextSize(1);

  ConectarWifi();
  ConectarWS();
}

void loop() {
  client.poll();
  M5.IMU.getAccelData(&accX, &accY, &accZ);
  bool sentMsg = false;

  // land / take off
  if (M5.BtnA.wasPressed()) {
    if (isOnAir) {
      client.send("land");
      delay(land_time);
    } else {
      client.send("takeoff");
      delay(takeoff_time);
    }
    sentMsg = true;
    isOnAir = !isOnAir;
  }
  
  // change state
  if (M5.BtnB.wasPressed()) {
    movementState += 1;
    if (movementState == 3) movementState = 0; 
  }

  if (isOnAir) {
    if (movementState == 0) {
      String x = "0", y = "0";

      if (accY > max_sine) {
        y = "-20"; // down
      } else if (accY < -max_sine) {
        y = "20"; // up
      }
      
      if (accX > max_sine) {
        x = "-20"; // left
      } else if (accX < -max_sine){
        x = "20"; // right
      }

      if (x != "0" || y != "0") {
        client.send("go " + x + " " + y + " 0 30");
        sentMsg = true;
      }
    } else if (movementState == 1) {
      if (accY > max_sine) {
        client.send("down 20");
        sentMsg = true;
      } else if (accY < -max_sine) {
        client.send("up 20");
        sentMsg = true;
      }
      
      if (accX > max_sine) {
        client.send("cw 10");
        sentMsg = true;
      } else if (accX < -max_sine){
        client.send("ccw 10");
        sentMsg = true;
      }
    } else {
        // steady state
    }
  } 

  if (!sentMsg) {
    client.send("stop"); // keep-alive
  }

  delay(pool_time);
  M5.update();
}
