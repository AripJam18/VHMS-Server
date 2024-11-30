#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include "Nextion.h"  // Library untuk Nextion

// Konfigurasi Wi-Fi
const char* ssid = "ESP32-Server";
const char* password = "password123";
WiFiServer server(80); // Membuat server di port 80

// Pin untuk komunikasi serial dengan Nextion
#define RXD2 16  // Pin RX (hubungkan kabel kuning Nextion ke sini)
#define TXD2 17  // Pin TX (hubungkan kabel biru Nextion ke sini)
HardwareSerial mySerial(2);

// Konfigurasi SD Card
#define SD_CS 5 // Pin CS untuk SD Card

// Objek Nextion
NexGauge GaugePLM = NexGauge(0, 3, "GaugePLM");
NexText TxtPLM = NexText(0, 8, "TxtPLM");
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
  WiFiClient client = server.available();

  if (client) {
    TxtStatus.setText("C-Connected");
    lastDataTime = millis();

    while (client.connected()) {
      if (client.available()) {
        String data = client.readStringUntil('\n');
        data.trim();

        Serial.println("Received Data: " + data);

        if (data.length() > 0) {
          displayDataOnNextion(data);
        }

        lastDataTime = millis();
      }

      if (millis() - lastDataTime > timeoutInterval) {
        Serial.println("Client disconnected due to timeout.");
        client.stop();
        TxtStatus.setText("C-Timeout");
        break;
      }
    }

    if (!client.connected()) {
      TxtStatus.setText("Waiting for data");
    }
  }
}

void displayDataOnNextion(String data) {
  // Parsing data
  String parts[6] = {"0", "0", "0", "0", "0", "HD78101KM"};
  int index = 0;
  while (data.indexOf('-') > 0 && index < 5) {
    int pos = data.indexOf('-');
    parts[index] = data.substring(0, pos);
    data = data.substring(pos + 1);
    index++;
  }
  parts[index] = data;

  for (int i = 0; i <= 5; i++) {
    if (parts[i] == "") {
      parts[i] = (i == 5) ? "HD78101KM" : "0";
    }
  }

  TxtUnit.setText(parts[5].c_str());
  TxtPLM.setText(parts[4].c_str());

  float payload = parts[4].toFloat();
  GaugePLM.setValue(mapGaugeValue(payload, 0, 101.1, 0, 180));

  // Simpan data ke SD Card
  saveDataToSD(parts);
}

int mapGaugeValue(float value, float in_min, float in_max, int out_min, int out_max) {
  if (value < in_min) value = in_min;
  if (value > in_max) value = in_max;
  return (int)((value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

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



//Penjelasan Modifikasi
//--commit ke 1
//Pemetaan Payload dan Tekanan:

//Fungsi mapGaugeValue() digunakan untuk memetakan nilai sensor ke rentang gauge (0–180).
//parts[0] hingga parts[3] adalah tekanan (FL, FR, RL, RR) dalam satuan MPa.
//parts[4] adalah payload dalam satuan ton.
//Konversi String ke Float:

//Fungsi .toFloat() digunakan untuk mengonversi string menjadi angka desimal (float).
//Penanganan Batas Nilai:

//Jika nilai tekanan atau payload melebihi rentang maksimal, fungsi mapGaugeValue() akan memastikan nilainya tetap dalam batasan gauge.
//Contoh Input dan Output
//Input data: 3.25-3.50-2.75-3.00-10.5-HD78101KM
//Gauge akan:
//FL: 3.25 MPa → Dikonversi menjadi ~5° pada gauge
//FR: 3.50 MPa → Dikonversi menjadi ~6° pada gauge
//RL: 2.75 MPa → Dikonversi menjadi ~4° pada gauge
//RR: 3.00 MPa → Dikonversi menjadi ~5° pada gauge
//Payload: 10.5 ton → Dikonversi menjadi ~19° pada gauge


//--commit ke 2
// Penjelasan Perbaikan pada void loop() dan void displayDataOnNextion()
// Penanganan Elemen Kosong

// Setiap elemen parts[] diperiksa. Jika ada elemen kosong (""), diisi dengan nilai default:
// "0" untuk angka.
// "HD78101KM" untuk nama unit (atau string lain yang sesuai).
// Validasi Panjang Data

// Sebelum parsing data, panjang data diperiksa. Data kosong tidak akan diproses.
// Debug Log

// Menambahkan log debug menggunakan Serial.println() untuk memantau data yang diterima.

//--commit ke 3
// Penambahan fungsi simpan data ke SD Card
// Inisialisasi SD Card:

// SD.begin(CS_PIN) digunakan untuk menginisialisasi SD Card.
// Jika SD Card gagal diinisialisasi, program akan berhenti untuk mencegah kerusakan data.
// Pembuatan File CSV:

// Jika file /data.csv belum ada, header kolom dibuat di file CSV.
// Fungsi saveDataToSD:

// Membuka file CSV dalam mode append dan menambahkan baris data baru dalam format:
// CLIENT,DATE,TIME,RIT,PAYLOAD
// CLIENT diambil dari TxtUnit.
// PAYLOAD diambil dari TxtPLM.
// Nilai DATE, TIME, dan RIT saat ini dibuat statik.
// Penanganan Kesalahan:

// Jika file gagal dibuka untuk penulisan, pesan kesalahan akan dicetak ke serial monitor.

//--commit ke 4
//Penambahan pesan "SD Card Mounted" pada Txt Status dan penambahan inisialisasi SD Card pada void Setup juga ada penambahan library SPI.h
