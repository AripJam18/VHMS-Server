#include <WiFi.h>
#include "Nextion.h"  // Library untuk Nextion
#include <SD.h>  // Library SD card
#include <SPI.h>

// Konfigurasi Wi-Fi
const char* ssid = "ESP32-Server";
const char* password = "password123";

WiFiServer server(80);  // Membuat server di port 80

// Pin untuk komunikasi serial dengan Nextion
#define RXD2 16  // Pin RX (hubungkan kabel kuning Nextion ke sini)
#define TXD2 17  // Pin TX (hubungkan kabel biru Nextion ke sini)
HardwareSerial mySerial(2);
// Konfigurasi SD Card
#define SD_CS 5 // Pin CS untuk SD Card
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

  // Wi-Fi Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Started");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("IP Address: ");
  Serial.println(IP);

  // Server start
  server.begin();
  Serial.println("Waiting for data...");

  // Nextion
  mySerial.begin(9600, SERIAL_8N1, RXD2, TXD2);  // UART untuk Nextion
  nexSerial = mySerial;
  nexInit();
  TxtStatus.setText("System Started");

  // Inisialisasi SPI dan SD card
  SPI.begin(18, 19, 23, SD_CS); // SCK, MISO, MOSI, CS
  if (SD.begin(SD_CS)) {
    Serial.println("SD card successfully mounted.");
  } else {
    Serial.println("Failed to mount SD card.");
    TxtStatus.setText("SD Mount Failed");
  }
}

void loop() {    
        // Proses data dan langsung tampilkan di Nextion
        displayDataOnNextion();
        delay(2000);
 
}

void displayDataOnNextion() {
  // Menghasilkan nilai random untuk tekanan (pressure)
  float fl = random(0, 10000) / 100.0; // 0.00 - 99.99 MPa
  float fr = random(0, 10000) / 100.0;
  float rl = random(0, 10000) / 100.0;
  float rr = random(0, 10000) / 100.0;
  float pl = random(0, 10110) / 100.0; // 0.00 - 101.10 T

  // Nama dump truck tetap
  String unitName = "HD78101KM";

  // Membuat string data dengan format sesuai
  String data = String(fl, 2) + "-" + 
                String(fr, 2) + "-" + 
                String(rl, 2) + "-" + 
                String(rr, 2) + "-" + 
                String(pl, 2) + "-" + 
                unitName;

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

  // // Tampilkan data pada Nextion
  // TxtUnit.setText(parts[5].c_str());  // Nama dump truck
  // TxtPLM.setText(parts[4].c_str());  // Payload

  // // Konversi dan peta nilai sensor ke gauge
  // TxtFL.setText(parts[0].c_str());
  // GaugeFL.setValue(mapPressureToGauge(parts[0].toFloat(), 99.99));

  // TxtFR.setText(parts[1].c_str());
  // GaugeFR.setValue(mapPressureToGauge(parts[1].toFloat(), 99.99));

  // TxtRL.setText(parts[2].c_str());
  // GaugeRL.setValue(mapPressureToGauge(parts[2].toFloat(), 99.99));

  // TxtRR.setText(parts[3].c_str());
  // GaugeRR.setValue(mapPressureToGauge(parts[3].toFloat(), 99.99));

  // TxtPLM.setText(parts[4].c_str());
  // GaugePLM.setValue(mapPressureToGauge(parts[4].toFloat(), 101.1));

    // Simpan data ke SD Card
  saveDataToSD(parts);
}

//fungsi simpan data ke SD CARD
void saveDataToSD(String parts[]) {
  // Format: CLIENT,DATE,TIME,RIT,PAYLOAD
  const char* filename = "/data.csv";
  File file = SD.open(filename, FILE_APPEND);

  if (!file) {
    Serial.println("Failed to open file for writing.");
    TxtStatus.setText("SD Write Failed");
    return;
  }

  String date = "30-11-2024";
  String time = "09:30";
  String rit = "1";
  String client = parts[5];
  String payload = parts[4] + "t";

  String dataLine = client + "," + date + "," + time + "," + rit + "," + payload + "\n";
  file.print(dataLine);

  Serial.println("Data saved to SD card: " + dataLine);
  TxtStatus.setText("Data Saved");
  file.close();
}


// Fungsi untuk memetakan nilai sensor ke rentang gauge
int mapPressureToGauge(float sensorValue, float maxSensorValue) {
  if (sensorValue > maxSensorValue) {
    sensorValue = maxSensorValue; // Batas atas
  } else if (sensorValue < 0) {
    sensorValue = 0; // Batas bawah
  }

  return (sensorValue / maxSensorValue) * 180; // Peta ke rentang 0–180
}


//ini untuk tes menampilkan nilai FL,RL,FR,RR dan PL ke dalam Text dan Gauge
//Penjelasan Kode
//Nilai Random:

//Nilai tekanan (FL, RL, FR, RR) dihasilkan secara acak antara 0.00–99.99 MPa.
//Nilai payload (PLM) dihasilkan secara acak antara 0.00–101.10 T.
//Pemetaan Nilai Sensor ke Gauge:

//Fungsi mapPressureToGauge memetakan nilai sensor ke rentang gauge (0–180).
//Jika nilai sensor lebih besar dari batas maksimum (misalnya 99.99 MPa untuk tekanan), maka nilai akan dibatasi pada maksimum.
//Penerapan Fungsi:

//Fungsi mapPressureToGauge digunakan untuk setiap nilai tekanan dan payload sebelum diatur ke gauge.
//Hasil
//Gauge pada Nextion akan menunjukkan nilai sesuai skala, di mana:

//Nilai 0.00 MPa ditampilkan sebagai 0 pada gauge.
//Nilai 99.99 MPa ditampilkan sebagai 180 pada gauge.
//Payload juga dipetakan ke rentang 0–180 sesuai skala 0–101.10 T.

//--commit ke 3
// penambahan fungsi simpan data ke sdcard
// comment tampilan ke nextion karena tes simpan data di rumah tidak dihubungkan ke nextion dulu

//--commit ke 4
//Penambahan pesan "SD Card Mounted" pada Txt Status dan penambahan inisialisasi SD Card pada void Setup juga ada penambahan library SPI.h


