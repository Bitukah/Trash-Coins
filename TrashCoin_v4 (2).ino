#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32QRCodeReader.h>
#include <Arduino_JSON.h>
#include <LiquidCrystal_I2C.h>

#define SDA 15
#define SCL 14
#define sensorInd 13
#define sensorCap 2
#define motorEsq 12
#define motorDir 4

// SDA 14
// SCL 15

#define WIFI_SSID "Douglas"
#define WIFI_PASSWORD "123456789."

String endPoint = "http://192.168.122.1:8080/apitt4a/";
String qrcode = "sem qrcode";
int codRetorno = 0;

ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER);
LiquidCrystal_I2C lcd(0x27, 16, 2);
struct QRCodeData qrCodeData;
bool isConnected = false;

float peso = 1.0;
String material = "VAZIO";
unsigned int delayMaterial = millis();



bool connectWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int maxRetries = 10;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    maxRetries--;
    if (maxRetries <= 0) {
      return false;
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Conectado!  ");
  delay(1000);
  return true;
}

int salvarReciclado(String registro, String material) {
  HTTPClient http;
  String url = String(endPoint + "reciclado?");
  url = String(url + "registro=");
  url = String(url + registro);
  url = String(url + "&material=");
  url = String(url + material);
  url = String(url + "&peso=");
  url = String(url + peso);
  if (http.begin(url.c_str())) {   // Consulta se o usuário existe no banco de dados
    int httpCode = http.POST("");  //
    if (httpCode > 0) {            // Recupera o código HTTP de retorno da requisição
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_CREATED) {
        String payload = http.getString();
        JSONVar json = JSON.parse(payload);
        if (JSON.typeof(json) == "undefined") {
          Serial.println("Json response undefined!");
        }
        JSONVar jsonKeys = json.keys();
        String nome = json[jsonKeys[2]];
        long credito = json[jsonKeys[5]];
        Serial.print("Nome: ");
        Serial.println(nome);
        Serial.print("Crédito: ");
        Serial.println(credito);
      }
      return httpCode;
    } else {
      Serial.printf("[HTTP] GET... falha, erro: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("[HTTP] Impossível de conectar");
  }
  return 0;
}

void readQrCode() {
  if (reader.receiveQrCode(&qrCodeData, 100)) {
    Serial.println("Found QRCode");
    if (qrCodeData.valid) {
      Serial.print("Payload: ");
      Serial.println((const char *)qrCodeData.payload);
      qrcode = String((const char *)qrCodeData.payload);
    } else {
      Serial.print("Invalid: ");
      Serial.println((const char *)qrCodeData.payload);
    }
  } else {
     qrcode = "sem qrcode";
  }
  delay(500);
}

void identificarMaterial() {
  if ((millis() - delayMaterial) < 7000) {
    //digitalWrite(flash, HIGH);
    return;
  }
  //digitalWrite(flash, LOW);
  if (digitalRead(sensorCap)) {
    qrcode = "sem qrcode";
    if (digitalRead(sensorInd)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("    ALUMINIO    ");
      material = "ALUMINIO";
      Serial.println("Alumínio detectado!");
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("    PLASTICO    ");
      material = "PLASTICO";
      Serial.println("Plástico detectado!");
    }
    lcd.setCursor(0, 1);
    lcd.print("Apresente QRCODE");
  } else {
    material = "VAZIO";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("  APRESENTE O  ");
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("  RECICLADO   ");
  }
  if (!material.equals("VAZIO")) {
    delayMaterial = millis();
  }
}


void setup() {
  Wire.begin(SDA, SCL);
  lcd.init();
  Serial.begin(115200);
  lcd.clear();           
  lcd.backlight();
  Serial.println();
  //pinMode(flash, OUTPUT);
  pinMode(sensorInd, INPUT);
  pinMode(sensorCap, INPUT);
  pinMode(motorEsq, OUTPUT);
  pinMode(motorDir, OUTPUT);
  digitalWrite(motorEsq, HIGH);
  digitalWrite(motorDir, HIGH);
  reader.setup();
  //reader.setDebug(true);
  Serial.println("Setup QRCode Reader");

  reader.begin();
  Serial.println("Begin QR Code reader");

  delay(1000);
}

void loop() {
  bool connected = connectWifi();
  if (isConnected != connected) {
    isConnected = connected;
  }
  identificarMaterial();
  if (!material.equals("VAZIO")) {
    readQrCode();
    if (!qrcode.equals("sem qrcode")) {
      codRetorno = salvarReciclado(qrcode, material);  //Exemplo de envio de reciclado
      if (codRetorno == 201) {
        if (material.equals("PLASTICO")) {
          //Serial.println("Motor DIR LOW");
          digitalWrite(motorDir, LOW);
          delay(250);  //tempo de giro do motor
          digitalWrite(motorDir, HIGH);
          delay(1500);
         // Serial.println("Motor DIR HIGH");
          digitalWrite(motorEsq, LOW);
          delay(250);  //tempo de giro do motor
          digitalWrite(motorEsq, HIGH);

        } else if (material.equals("ALUMINIO")) {
          digitalWrite(motorEsq, LOW);
          delay(250);  //tempo de giro do motor
          digitalWrite(motorEsq, HIGH);
          delay(1500);
          digitalWrite(motorDir, LOW);
          delay(250);  //tempo de giro do motor
          digitalWrite(motorDir, HIGH);
        }
        delayMaterial = 0;
        material = "VAZIO";
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("    Usuario    ");
        lcd.setCursor(0, 1);
        lcd.print("  Sem cadastro ");
        Serial.println("Sem Cadastro");
        delayMaterial = 0;
      }
       qrcode = "sem qrcode";
    }
  }
}

