#include <WiFi.h>
#include <WebServer.h>
#include <BTS7960.h>
#include <FS.h>
#include <SPIFFS.h>

// Motor 1 pins (kiri)
#define L_EN 21
#define R_EN 19
#define L_PWM 18
#define R_PWM 5

// Motor 2 pins (kanan)
#define L2_EN 27
#define R2_EN 26
#define L2_PWM 25
#define R2_PWM 33

// Motor objects
BTS7960 motor1(L_EN, R_EN, L_PWM, R_PWM);
BTS7960 motor2(L2_EN, R2_EN, L2_PWM, R2_PWM);

const char* ssid = "RASALIpoh";
const char* password = "saranghae";

WebServer server(80);

// Status motor saat ini
String currentDir = "stop";
int currentSpeed = 0;

void handleControl() {
  if (!server.hasArg("dir") || !server.hasArg("speed")) {
    server.send(400, "text/plain", "Missing parameters");
    return;
  }

  String dir = server.arg("dir");
  int speed = constrain(server.arg("speed").toInt(), 0, 255);

  if (dir != "forward" && dir != "backward" && dir != "stop" && dir != "right" && dir != "left") {
    server.send(400, "text/plain", "Invalid dir parameter");
    return;
  }

  Serial.printf("Received command: dir=%s, speed=%d\n", dir.c_str(), speed);

  // Atur PWM motor
  motor1.pwm = speed;
  motor2.pwm = speed;

  // Logic arah
  if (dir == "forward") {
    motor1.front();  // Motor kiri maju
    motor2.front();  // Motor kanan maju
  } else if (dir == "backward") {
    motor1.back();   // Motor kiri mundur
    motor2.back();   // Motor kanan mundur
  } else if (dir == "right") {
    motor1.front();  // Motor kiri maju
    motor2.back();   // Motor kanan mundur
  } else if (dir == "left") {
    motor1.back();   // Motor kiri mundur
    motor2.front();  // Motor kanan maju
  } else if (dir == "stop") {
    motor1.stop();
    motor2.stop();
  }

  // Update status
  currentDir = dir;
  currentSpeed = (dir == "stop") ? 0 : speed;

  server.send(200, "text/plain", "OK");
}

void handleRoot() {
  File file = SPIFFS.open("/index.html");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleStatus() {
  String json = "{";
  json += "\"dir\":\"" + currentDir + "\",";
  json += "\"speed\":" + String(currentSpeed);
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  // Init Motors
  motor1.begin();
  motor1.enable();

  motor2.begin();
  motor2.enable();

  // Init SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 30) {
    delay(500);
    Serial.print(".");
    timeout++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi");
  } else {
    Serial.println("\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }

  // Setup routes
  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.on("/status", handleStatus);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
