#include <WiFi.h>

const char* ssid = "ESP32-Server";
const char* password = "password123";
const char* host = "192.168.4.1";  // IP address of the server

#define RX2 16
#define TX2 17
#define BUTTON_START 18  // Pin untuk tombol start
#define BUTTON_STOP 19   // Pin untuk tombol stop

// Tambahkan definisi pin untuk LED
#define LED_RED 21      // Untuk status IDLE atau DISCONNECTED
#define LED_YELLOW 22   // Untuk status CONNECTING
#define LED_GREEN 23    // Untuk status TRANSMITTING

WiFiClient client;
String serialData = "";  // Variabel untuk menyimpan data dari Serial2
bool shouldSendData = false;  // Flag untuk mengatur apakah data harus dikirim
unsigned long startTime = 0;  // Untuk menghitung waktu autostop

// State machine status
enum State {
  IDLE,
  CONNECTING,
  TRANSMITTING,
  DISCONNECTED
};

State currentState = IDLE;  // Mulai dengan status idle

void reconnect() {
  // Fungsi untuk mencoba reconnect ke server
  Serial.println("Trying to reconnect to server...");
  
  int maxRetries = 5;  // Batas maksimal percobaan reconnect
  int retries = 0;
  
  while (!client.connect(host, 80)) {
    retries++;
    Serial.println("Reconnecting to server... Attempt: " + String(retries));
    delay(1000);  // Coba reconnect setiap 1 detik
    
    if (retries >= maxRetries) {
      Serial.println("Failed to connect to server after " + String(maxRetries) + " attempts.");
      currentState = DISCONNECTED;  // Jika gagal, kembali ke state disconnected
      return;
    }
  }

  Serial.println("Connected to server.");
  currentState = TRANSMITTING;  // Berhasil, pindah ke state transmitting
}

void stopConnection() {
  // Fungsi untuk autostop dan disconnect
  Serial.println("Stopping data transmission and disconnecting...");
  
  shouldSendData = false;  // Hentikan pengiriman data

  if (client.connected()) {
    client.stop();  // Disconnect dari server
    Serial.println("Disconnected from server.");
  }

  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();  // Disconnect dari WiFi
    Serial.println("Disconnected from WiFi.");
  }

  delay(1000);  // Tambahkan sedikit delay untuk memastikan koneksi benar-benar diputus

  currentState = IDLE;  // Pastikan kembali ke state IDLE
  Serial.println("Current state: IDLE");
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RX2, TX2);

  // Inisialisasi tombol
  pinMode(BUTTON_START, INPUT_PULLUP);
  pinMode(BUTTON_STOP, INPUT_PULLUP);
  
  // Inisialisasi LED sebagai output
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  // Mulai dengan semua LED mati
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_GREEN, LOW);
  
  Serial.println("System ready. Press START to connect.");
}

void loop() {
  switch (currentState) {
    case IDLE:
      // Menyalakan LED merah untuk status IDLE
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, LOW);
      
      // Menunggu tombol start ditekan
      if (digitalRead(BUTTON_START) == LOW) {
        Serial.println("Connecting to WiFi...");
        currentState = CONNECTING;  // Pindah ke state CONNECTING
        Serial.println("Current state: CONNECTING");
      }
      break;

    case CONNECTING: {
      // Menyalakan LED kuning untuk status CONNECTING
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_YELLOW, HIGH);
      digitalWrite(LED_GREEN, LOW);
      
      // Timeout untuk WiFi connection
      unsigned long wifiTimeout = millis();
      WiFi.begin(ssid, password);

      while (WiFi.status() != WL_CONNECTED && millis() - wifiTimeout < 10000) {  // Timeout 10 detik
        delay(1000);
        Serial.println("Connecting to WiFi...");
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi.");
        reconnect();  // Jika WiFi terhubung, coba connect ke server
        startTime = millis();  // Set waktu mulai
        shouldSendData = true;  // Aktifkan pengiriman data
      } else {
        Serial.println("WiFi connection failed. Returning to IDLE.");
        currentState = IDLE;  // Jika WiFi gagal terhubung, kembali ke state idle
        Serial.println("Current state: IDLE");
      }
      break;
    }

    case TRANSMITTING:
      // Menyalakan LED hijau untuk status TRANSMITTING
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, HIGH);
      
      // Cek tombol stop untuk menghentikan koneksi
      if (digitalRead(BUTTON_STOP) == HIGH) {
        stopConnection();  // Panggil fungsi stopConnection jika tombol stop ditekan
      }

      // Membaca data dari Serial2 jika ada
      if (Serial2.available()) {
        serialData = Serial2.readStringUntil('\n');  // Baca data dari Serial2
        Serial.println("Data from Serial2: " + serialData);
      }

      // Kirim data ke server jika terhubung dan ada data
      if (client.connected() && shouldSendData && serialData.length() > 0) {
        unsigned long sendTimeout = millis();
        if (client.println(serialData)) {
          Serial.println("Sent to server: " + serialData);
        } else if (millis() - sendTimeout > 1000) {  // Timeout 3 detik
          Serial.println("Failed to send data to server. Reconnecting...");
          reconnect();
        }
        serialData = "";  // Kosongkan buffer setelah pengiriman
      }

      // Tambahkan pengecekan berkala untuk memastikan koneksi masih aktif
      static unsigned long lastCheck = 0;
      if (millis() - lastCheck > 1000) {  // Cek setiap 1 detik
        lastCheck = millis();
        if (!client.connected()) {
          Serial.println("Connection to server lost. Reconnecting...");
          reconnect();
        }
      }
      break;

    case DISCONNECTED:
      // Menyalakan LED merah untuk status DISCONNECTED
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_GREEN, LOW);
      
      stopConnection();  // Panggil fungsi stopConnection saat status DISCONNECTED
      currentState = IDLE;  // Kembali ke state IDLE setelah disconnect
      Serial.println("Current state: IDLE");
      break;
  }
}
