#include <WiFi.h>
#include <Arduino_JSON.h>
#include <Servo.h>

WiFiClient client;

// 서보모터 핀 정의
#define PLASTIC_SERVO_PIN 9
#define CAN_SERVO_PIN 10
#define PAPER_SERVO_PIN 11

// 적외선 센서 핀 정의
#define PLASTIC_IR_SENSOR_PIN 2
#define CAN_IR_SENSOR_PIN 3
#define PAPER_IR_SENSOR_PIN 4

// 초음파 센서 핀 정의 (각 쓰레기통마다 트리거 및 에코 핀 설정)
#define PLASTIC_TRIGGER_PIN 5
#define PLASTIC_ECHO_PIN 6
#define CAN_TRIGGER_PIN 7
#define CAN_ECHO_PIN 8
#define PAPER_TRIGGER_PIN 12
#define PAPER_ECHO_PIN 13

Servo plasticServo;
Servo canServo;
Servo paperServo;

#define OPEN_DURATION 14500 // 10초 타이머

const char* ssid = "iPhonekjc";
const char* password = "12345678**";
const unsigned long wifiTimeout = 10000;

const char* serverName1 = "3.38.54.56"; // 서버 IP 주소
const int port = 5000; // 서버 포트 번호
const char* webServerName = "172.20.10.4";      
const int webPort = 8000;

WiFiServer server(80); // Arduino HTTP 서버 설정

void setup() {
  Serial.begin(115200);
  delay(5000);
  int n = WiFi.scanNetworks();
  Serial.println("Scan complete.");
  if (n == 0) {
    Serial.println("No networks found.");
  } else {
    Serial.println("Networks found:");
    for (int i = 0; i < n; ++i) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.println(" dBm)");
      delay(10);

      
    }
  }
  delay(1000);
  Serial.println("Attempting to connect to WiFi...");
  WiFi.begin(ssid, password);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime >= wifiTimeout) {
      Serial.println("WiFi connection failed. Retrying...");
      int n = WiFi.scanNetworks();
  Serial.println("Scan complete.");
  if (n == 0) {
    Serial.println("No networks found.");
  } else {
    Serial.println("Networks found:");
    for (int i = 0; i < n; ++i) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.println(" dBm)");
      delay(10);

      
    }
  }
      startTime = millis();
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
    delay(1000);
  }

  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  plasticServo.attach(PLASTIC_SERVO_PIN);
  canServo.attach(CAN_SERVO_PIN);
  paperServo.attach(PAPER_SERVO_PIN);

  pinMode(PLASTIC_IR_SENSOR_PIN, INPUT);
  pinMode(CAN_IR_SENSOR_PIN, INPUT);
  pinMode(PAPER_IR_SENSOR_PIN, INPUT);

  pinMode(PLASTIC_TRIGGER_PIN, OUTPUT);
  pinMode(PLASTIC_ECHO_PIN, INPUT);
  pinMode(CAN_TRIGGER_PIN, OUTPUT);
  pinMode(CAN_ECHO_PIN, INPUT);
  pinMode(PAPER_TRIGGER_PIN, OUTPUT);
  pinMode(PAPER_ECHO_PIN, INPUT);

  server.begin();
}

void openTrashCan(String trashType) {
      // HTTP 200 OK 응답 전송
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println("{\"status\":\"received\",\"message\":\"Trash can opened successfully\"}");
        // 응답이 전송되었음을 Serial에 출력
    Serial.println("HTTP Response sent to client.");
    client.stop();   // 클라이언트 연결 종료
    Serial.println("Client connection closed.");
  if (trashType == "Can") {
    canServo.write(80);
    Serial.println("Opening Can trash can");
  } else if (trashType == "Plastic") {
    plasticServo.write(80);
    Serial.println("Opening Plastic trash can");
  } else if (trashType == "Paper") {
    paperServo.write(80);
    Serial.println("Opening Paper trash can");
  }
}

void closeTrashCan(String trashType) {
  canServo.write(0);
  plasticServo.write(0);
  paperServo.write(0);
  Serial.println("Closing trash can");

  // 문이 닫힌 후 초음파 센서 값을 측정하여 포화 수준 계산 및 전송
  int fillLevel = measureFillLevel(trashType);
  sendFillLevel(trashType, fillLevel);
}

bool checkTrash(String trashType) {
  bool trashDetected = false;

  if (trashType == "Can") {
    trashDetected = digitalRead(CAN_IR_SENSOR_PIN) == LOW;
    Serial.print("Can IR Sensor detected trash: ");
    Serial.println(trashDetected);
  } else if (trashType == "Plastic") {
    trashDetected = digitalRead(PLASTIC_IR_SENSOR_PIN) == LOW;
    Serial.print("Plastic IR Sensor detected trash: ");
    Serial.println(trashDetected);
  } else if (trashType == "Paper") {
    trashDetected = digitalRead(PAPER_IR_SENSOR_PIN) == LOW;
    Serial.print("Paper IR Sensor detected trash: ");
    Serial.println(trashDetected);
  }
  
  return trashDetected;
}

void sendTrashStatus(int userId, String trashType, bool trashDetected) {
  // 기존 서버로 데이터 전송
  WiFiClient client1; // 새로운 WiFiClient 인스턴스
  if (client1.connect(serverName1, port)) {
    String postData = "{\"user_id\":" + String(userId) + ",\"trash_type\":\"" + trashType + "\",\"trash_boolean\":" + (trashDetected ? "true" : "false") + "}";
    client1.println("POST /recycle/add_points HTTP/1.1");
    client1.println("Host: " + String(serverName1));
    client1.println("Content-Type: application/json");
    client1.print("Content-Length: ");
    client1.println(postData.length());
    client1.println();
    client1.println(postData);
    Serial.println("[포인트 적립 요청]");
    // 응답 확인
    while (client1.connected()) {
      String line = client1.readStringUntil('\n');
      if (line == "\r") break;
      Serial.println(line);
    }
    client1.stop();
  } else {
    Serial.println("포인트 적립 요청 실패");
  }

  // 새로운 Flask 서버로 trashdetected 상태 전송
  WiFiClient client2;  // 새로운 WiFiClient 인스턴스
if (client2.connect(webServerName, webPort)) {  // Flask 서버 IP와 포트 설정
    String flaskPostData = "{\"trash_inserted\":" + String(trashDetected ? "true" : "false") + "}";
    Serial.println("HTTP Request:");
    Serial.print("POST /trash_successful HTTP/1.1\r\n");
    Serial.print("Host: ");
    Serial.println(webServerName);
    Serial.print("Content-Type: application/json\r\n");
    Serial.print("Content-Length: ");
    Serial.println(flaskPostData.length());
    Serial.println(flaskPostData);

    client2.println("POST /trash_successful HTTP/1.1");
    client2.println("Host: " + String(webServerName));
    client2.println("Content-Type: application/json");
    client2.print("Content-Length: ");
    client2.println(flaskPostData.length());
    client2.println();
    client2.println(flaskPostData);
    client2.println();

    Serial.println("[웹페이지 쓰레기 배출 여부 전송]");
    // 응답 확인
    while (client2.connected() || client2.available()) {
        if (client2.available()) {
            String line = client2.readStringUntil('\n');
            Serial.println(line);
        }
    }
    client2.stop();
} else {
    Serial.println("웹 화면 쓰레기 여부 요청 실패");
}

}


void sendFillLevel(String trashType, int fillLevel) {
  WiFiClient awsClient; // AWS 서버로 데이터를 보내기 위한 별도 WiFiClient
  if (awsClient.connect(serverName1, port)) {  // AWS 서버 연결
    String postData = "{\"trash_type\":\"" + trashType + "\",\"fill_level\":" + String(fillLevel) + "}";
    awsClient.println("POST /trash_bin/update_fill_level HTTP/1.1");
    awsClient.println("Host: " + String(serverName1));
    awsClient.println("Content-Type: application/json");
    awsClient.print("Content-Length: ");
    awsClient.println(postData.length());
    awsClient.println();
    awsClient.println(postData);
    Serial.println("[포화도 전송]");

    // 응답 확인
    while (awsClient.connected()) {
      String line = awsClient.readStringUntil('\n');
      if (line == "\r") break; // 빈 줄은 헤더 끝
      Serial.println(line);
    }
    awsClient.stop(); // 연결 종료
  } else {
    Serial.println("포화도 전송 요청 실패");
  }
}


int measureFillLevel(String trashType) {
  long duration, distance;
  int maxDistance = 18; // 쓰레기통의 최대 깊이 (센티미터 단위)

  if (trashType == "Plastic") {
    digitalWrite(PLASTIC_TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(PLASTIC_TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(PLASTIC_TRIGGER_PIN, LOW);
    duration = pulseIn(PLASTIC_ECHO_PIN, HIGH);
    Serial.print("Plastic Ultrasonic Distance: ");
  } else if (trashType == "Can") {
    digitalWrite(CAN_TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(CAN_TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(CAN_TRIGGER_PIN, LOW);
    duration = pulseIn(CAN_ECHO_PIN, HIGH);
    Serial.print("Can Ultrasonic Distance: ");
  } else if (trashType == "Paper") {
    digitalWrite(PAPER_TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(PAPER_TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(PAPER_TRIGGER_PIN, LOW);
    duration = pulseIn(PAPER_ECHO_PIN, HIGH);
    Serial.print("Paper Ultrasonic Distance: ");
  }

  distance = duration * 0.034 / 2; // 거리 계산 (cm)
  Serial.print(distance);
  Serial.println(" cm");

  // 포화도 계산 (거리가 작을수록 포화도가 높음)
  int fillLevel = map(distance, 0, maxDistance, 100, 0);
  fillLevel = constrain(fillLevel, 0, 100); // 포화도는 0~100% 사이로 제한

  Serial.print("Calculated fill level for ");
  Serial.print(trashType);
  Serial.print(": ");
  Serial.print(fillLevel);
  Serial.println("%");

  return fillLevel;
}

void handleRequest(String qrData) {
  int separatorIndex = qrData.indexOf(':');
  if (separatorIndex != -1) {
    int userId = qrData.substring(0, separatorIndex).toInt();
    String trashType = qrData.substring(separatorIndex + 1);

    Serial.print("Parsed user ID: ");
    Serial.println(userId);
    Serial.print("Parsed trash type: ");
    Serial.println(trashType);

    openTrashCan(trashType);

    
    unsigned long startTime = millis();
    bool trashDetected = false;
    delay(1000);

    // 10초 동안 적외선 센서 상태를 검사
    while (millis() - startTime < OPEN_DURATION) {
      if (checkTrash(trashType)) {
        trashDetected = true;
        break;
      }
    }

    // 상태에 따라 서버에 HTTP POST 요청 전송
    sendTrashStatus(userId, trashType, trashDetected);
    delay(2000);
    closeTrashCan(trashType); // 문을 닫으면서 포화도 측정 및 전송
  } else {
    Serial.println("Error: Invalid QR format");
  }
}

void loop() {
  // 시리얼 입력 처리
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "포화도") {
      Serial.println("Sending current fill levels for all trash bins...");
      
      int plasticFillLevel = measureFillLevel("Plastic");
      int canFillLevel = measureFillLevel("Can");
      int paperFillLevel = measureFillLevel("Paper");
      
      sendFillLevel("Plastic", plasticFillLevel);
      sendFillLevel("Can", canFillLevel);
      sendFillLevel("Paper", paperFillLevel);

      Serial.println("Fill levels sent!");
    } else {
      Serial.print("Unknown command: ");
      Serial.println(command);
    }
  }

  WiFiClient client = server.available();
if (client) {
  String request = client.readString();

  Serial.print("Full Received Data: ");
  Serial.println(request);

  int jsonStartIndex = request.indexOf("\r\n\r\n");
  if (jsonStartIndex == -1) {
    Serial.println("Failed to find JSON body in the data");

    // HTTP 응답 전송
    client.println("HTTP/1.1 400 Bad Request");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("Invalid request format");
    client.stop();
    return;
  }

  String jsonBody = request.substring(jsonStartIndex + 4);
  jsonBody.trim();

  Serial.print("Extracted JSON body: ");
  Serial.println(jsonBody);

  JSONVar parsedData = JSON.parse(jsonBody);

  if (JSON.typeof(parsedData) == "undefined") {
    Serial.println("Failed to parse JSON");

    // HTTP 응답 전송
    client.println("HTTP/1.1 400 Bad Request");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("JSON parse error");
    client.stop();
    return;
  }

  String qrData = (const char*)parsedData["qr_data"];
  Serial.print("Parsed qr_data: ");
  Serial.println(qrData);

  // QR 데이터 처리
  handleRequest(qrData);
}

}
