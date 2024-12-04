#include <WiFi.h>
#include "Nextion.h"  // Library untuk Nextion
#include <SD.h>  // Library SD card
#include <SPI.h>
#include <Wire.h>
#include "Adafruit_Thermal.h"


// Konfigurasi Wi-Fi
const char* ssid = "ESP32-Server";
const char* password = "password123";

WiFiServer server(80);  // Membuat server di port 80
//pin untuk komunikasi serial dengan Printer
#define RXD1 4  // Pin RX (hubungkan kabel biru Printer ke sini)
#define TXD1 5  // Pin TX (hubungkan kabel hijau Printer ke sini)
HardwareSerial printerSerial(1);
Adafruit_Thermal printer(&printerSerial);


// Pin untuk komunikasi serial dengan Nextion
#define RXD2 16  // Pin RX (hubungkan kabel kuning Nextion ke sini)
#define TXD2 17  // Pin TX (hubungkan kabel biru Nextion ke sini)
HardwareSerial mySerial(2);


// Konfigurasi SD Card
#define SD_CS 2 // Pin CS untuk SD Card , harusnya pin 5 tapi karena pin 5 dan 4 dipakai untuk UART jadi tuker ke pin 2
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

// --- Variabel Data Untuk Printer---
String last10Data[10];
int currentIndex = 0;
unsigned long lastPrintTime = 0;
const unsigned long printInterval = 60000; // 1 menit

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
  // Printer
  printerSerial.begin(9600, SERIAL_8N1, RXD1, TXD1); // pin 4 dan 5
  printer.begin();
  printer.justify('C');
  printer.setSize('M');
  printer.println("HD78140KM");
  printer.println("System Ready");
  printer.println("--------------------------");
  printer.sleep();
  
}

void loop() {    
// Periksa apakah waktunya mencetak data
  if (millis() - lastPrintTime > printInterval) {
    printLast10Data();
    lastPrintTime = millis();
  }

  // Tampilkan data pada Nextion
  displayDataOnNextion();

  delay(2000); // Hindari penggunaan perangkat bersamaan
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
   TxtUnit.setText(parts[5].c_str());  // Nama dump truck
   TxtPLM.setText(parts[4].c_str());  // Payload

  // // Konversi dan peta nilai sensor ke gauge
   TxtFL.setText(parts[0].c_str());
   GaugeFL.setValue(mapPressureToGauge(parts[0].toFloat(), 99.99));

   TxtFR.setText(parts[1].c_str());
   GaugeFR.setValue(mapPressureToGauge(parts[1].toFloat(), 99.99));

   TxtRL.setText(parts[2].c_str());
   GaugeRL.setValue(mapPressureToGauge(parts[2].toFloat(), 99.99));

   TxtRR.setText(parts[3].c_str());
   GaugeRR.setValue(mapPressureToGauge(parts[3].toFloat(), 99.99));

   TxtPLM.setText(parts[4].c_str());
   GaugePLM.setValue(mapPressureToGauge(parts[4].toFloat(), 101.1));

    // Simpan data ke SD Card
   saveDataToSD(parts[4], parts[5]);
   saveToLast10Data(parts[4], parts[5]);
}

//fungsi simpan data ke SD CARD
void saveDataToSD(String payload, String unitName) {
  File file = SD.open("/data.csv", FILE_APPEND);
  if (!file) {
    Serial.println("SD write failed.");
    TxtStatus.setText("SD Write Failed");
    return;
  }

  String date = "30-11-2024";
  String time = "09:30";
  String rit = "1";

  String dataLine = unitName + "," + date + "," + time + "," + rit + "," + payload + "t\n";
  file.print(dataLine);
  Serial.println("Saved to SD: " + dataLine);
  TxtStatus.setText("Data Saved");
  file.close();
}

void saveToLast10Data(String payload, String unitName) {
  last10Data[currentIndex] = unitName + "   1   " + payload + "t";
  currentIndex = (currentIndex + 1) % 10; // Circular buffer
}

//print 10 data terakhir
void printLast10Data() {
  printer.wake();
  printer.justify('L');
  printer.setSize('S');
  printer.println("TIME     RIT     PAYLOAD");
  printer.println("--------------------------");

  for (int i = 0; i < 10; i++) {
    int idx = (currentIndex + i) % 10;
    if (last10Data[idx].length() > 0) {
      printer.println(last10Data[idx]);
    }
  }

  printer.println("");
  printer.sleep();
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

//--commit ke 5
//Penambahan fungsi cetak dengan thermal printer dalam kode ini , data dicetak setiap 1 menit. data yang dicetak adalah 10 data randomvalue yg terakhir diterima


