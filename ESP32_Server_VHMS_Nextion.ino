#include <WiFi.h>
#include "Nextion.h"  // Library untuk Nextion

// Konfigurasi Wi-Fi
const char* ssid = "ESP32-Server";
const char* password = "password123";

WiFiServer server(80);  // Membuat server di port 80

// Pin untuk komunikasi serial dengan Nextion
#define RXD2 16  // Pin RX (hubungkan kabel kuning Nextion ke sini)
#define TXD2 17  // Pin TX (hubungkan kabel biru Nextion ke sini)
HardwareSerial mySerial(2);

// Objek Nextion untuk Gauge dan Text
NexGauge GaugeFL = NexGauge(0, 1, "GaugeFL");
NexText TxtFL = NexText(0, 6, "TxtFL");

NexGauge GaugeRL = NexGauge(0, 2, "GaugeRL");
NexText TxtRL = NexText(0, 7, "TxtRL");

NexGauge GaugePLM = NexGauge(0, 3, "GaugePLM");
NexText TxtPLM = NexText(0, 8, "TxtPLM");

NexGauge GaugeFR = NexGauge(0, 4, "GaugeFR");
NexText TxtFR = NexText(0, 9, "TxtFR");

NexGauge GaugeRR = NexGauge(0, 5, "GaugeRR");
NexText TxtRR = NexText(0, 10, "TxtRR");

NexText TxtUnit = NexText(0, 11, "TxtUnit");
NexText TxtStatus = NexText(0, 12, "TxtStatus");

// Variabel waktu untuk koneksi
unsigned long lastDataTime = 0;
const unsigned long timeoutInterval = 5000;  // Timeout 5 detik

void setup() {
  Serial.begin(115200);
  
  // Mengaktifkan mode Access Point (AP)
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Started");
  
  // Mendapatkan dan menampilkan alamat IP dari Access Point
  IPAddress IP = WiFi.softAPIP();
  Serial.print("IP Address: ");
  Serial.println(IP);
  
  // Memulai server
  server.begin();
  Serial.println("Waiting for data...");

  // Konfigurasi Nextion
  mySerial.begin(9600, SERIAL_8N1, RXD2, TXD2);  // UART untuk Nextion
  nexSerial = mySerial;
  nexInit();
  TxtStatus.setText("System Started");
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    TxtStatus.setText("C-Connected");  // Tampilkan status koneksi di Nextion
    lastDataTime = millis();  // Setel waktu koneksi dimulai
      
    while (client.connected()) {
      if (client.available()) {
        String data = client.readStringUntil('\n');
        data.trim();  // Menghapus spasi ekstra
        Serial.println("Received: " + data);

        lastDataTime = millis();  // Perbarui waktu terakhir data diterima
        
        // Proses data dan langsung tampilkan di Nextion
        displayDataOnNextion(data);
      }

      if (millis() - lastDataTime > timeoutInterval) {
        Serial.println("Client disconnected due to timeout.");
        client.stop();
        TxtStatus.setText("C-Timeout");  // Status timeout
        break;
      }
    }

    if (!client.connected()) {
      TxtStatus.setText("Waiting for data");  // Status menunggu data
    }
  }
}

void displayDataOnNextion(String data) {
  // Memisahkan data berdasarkan delimiter '-'
  int index = 0;
  String parts[6];
  while (data.indexOf('-') > 0 && index < 5) {
    int pos = data.indexOf('-');
    parts[index] = data.substring(0, pos);
    data = data.substring(pos + 1);
    index++;
  }
  parts[index] = data;

  // Tampilkan data pada Nextion
  TxtUnit.setText(parts[5].c_str());  // Nama dump truck
  TxtPLM.setText(parts[4].c_str());  // Payload

  TxtFL.setText(parts[0].c_str());
  GaugeFL.setValue(parts[0].toInt());

  TxtFR.setText(parts[1].c_str());
  GaugeFR.setValue(parts[1].toInt());

  TxtRL.setText(parts[2].c_str());
  GaugeRL.setValue(parts[2].toInt());

  TxtRR.setText(parts[3].c_str());
  GaugeRR.setValue(parts[3].toInt());
}
