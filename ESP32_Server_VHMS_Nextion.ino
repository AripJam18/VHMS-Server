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
        TxtStatus.setText("C-Connected");  // Status koneksi
        lastDataTime = millis();  // Setel waktu koneksi dimulai

        while (client.connected()) {
            if (client.available()) {
                String data = client.readStringUntil('\n');
                data.trim();  // Menghapus spasi ekstra

                // Debug untuk memeriksa data yang diterima
                Serial.println("Received Data: " + data);

                // Validasi panjang data
                if (data.length() > 0) {
                    displayDataOnNextion(data);
                } else {
                    Serial.println("Data kosong diterima, melewati update.");
                }

                lastDataTime = millis();  // Perbarui waktu terakhir data diterima
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
    // Array untuk menampung hasil parsing, dengan default nilai kosong
    String parts[6] = {"0", "0", "0", "0", "0", "HD78101KM"};  // Default values

    // Parsing data
    int index = 0;
    while (data.indexOf('-') > 0 && index < 5) {
        int pos = data.indexOf('-');
        parts[index] = data.substring(0, pos);
        data = data.substring(pos + 1);
        index++;
    }
    parts[index] = data; // Isi elemen terakhir

    // Pastikan semua bagian memiliki nilai (jika kosong, gunakan default)
    for (int i = 0; i <= 5; i++) {
        if (parts[i] == "") {
            parts[i] = (i == 5) ? "HD78101KM" : "0";  // Nama unit tetap, data lainnya default 0
        }
    }

    // Tampilkan data di Nextion
    TxtUnit.setText(parts[5].c_str());  // Nama dump truck
    TxtPLM.setText(parts[4].c_str());  // Payload

    float payload = parts[4].toFloat();
    GaugePLM.setValue(mapGaugeValue(payload, 0, 101.1, 0, 180));  // Pemetaan payload

    TxtFL.setText(parts[0].c_str());   // Tekanan FL
    float pressureFL = parts[0].toFloat();
    GaugeFL.setValue(mapGaugeValue(pressureFL, 0, 99.99, 0, 180));

    TxtFR.setText(parts[1].c_str());   // Tekanan FR
    float pressureFR = parts[1].toFloat();
    GaugeFR.setValue(mapGaugeValue(pressureFR, 0, 99.99, 0, 180));

    TxtRL.setText(parts[2].c_str());   // Tekanan RL
    float pressureRL = parts[2].toFloat();
    GaugeRL.setValue(mapGaugeValue(pressureRL, 0, 99.99, 0, 180));

    TxtRR.setText(parts[3].c_str());   // Tekanan RR
    float pressureRR = parts[3].toFloat();
    GaugeRR.setValue(mapGaugeValue(pressureRR, 0, 99.99, 0, 180));
}


// Fungsi untuk memetakan nilai sensor ke nilai gauge
int mapGaugeValue(float value, float in_min, float in_max, int out_min, int out_max) {
  if (value < in_min) value = in_min;
  if (value > in_max) value = in_max;
  return (int)((value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

//Penjelasan Modifikasi
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
