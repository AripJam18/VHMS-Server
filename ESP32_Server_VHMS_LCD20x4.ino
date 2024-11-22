#include <WiFi.h>
#include <LiquidCrystal_I2C.h>  // Library untuk LCD I2C

const char* ssid = "ESP32-Server";
const char* password = "password123";

WiFiServer server(80);  // Membuat server di port 80

// Setup untuk LCD I2C 20x4
LiquidCrystal_I2C lcd(0x27, 20, 4);  // Alamat I2C 0x27, ukuran 20x4

unsigned long lastDataTime = 0;  // Untuk mencatat waktu terakhir data diterima
const unsigned long timeoutInterval = 5000;  // Timeout 5 detik

// Fungsi untuk menambahkan spasi agar sesuai dengan batas karakter LCD
String padString(String data, int maxLength) {
  int dataLength = data.length();
  
  // Jika panjang data kurang dari maxLength, tambahkan spasi
  if (dataLength < maxLength) {
    int spacesToAdd = maxLength - dataLength;
    for (int i = 0; i < spacesToAdd; i++) {
      data += " ";  // Tambahkan spasi di akhir string
    }
  }
  
  // Jika data lebih panjang dari maxLength, potong sesuai batas
  if (dataLength > maxLength) {
    data = data.substring(0, maxLength);
  }

  return data;
}

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
  // Set kecepatan I2C menjadi 400 kHz
  Wire.setClock(400000);
  // Inisialisasi LCD I2C
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Started...");
  Serial.println("Waiting for data...");
  delay(1000);
  lcd.clear();
}

void loop() {
  // Menunggu klien yang terhubung
  WiFiClient client = server.available();
  
  if (client) {
    Serial.println("Client connected.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("    C-Connected    ");  // Tampilkan status koneksi di LCD
    lastDataTime = millis();  // Setel waktu koneksi dimulai
      
    while (client.connected()) {
      if (client.available()) {
        // Membaca data dari klien sampai ada new line ('\n')
        String data = client.readStringUntil('\n');
        data.trim();  // Menghapus spasi ekstra
        Serial.println("Received: " + data);

        // Perbarui waktu terakhir data diterima
        lastDataTime = millis();
        
        // Proses data dan langsung tampilkan di LCD
        displayDataOnLCD(data);
      }

      // Cek apakah sudah lebih dari 5+1 detik tidak ada data
      if (millis() - lastDataTime > timeoutInterval) {
        Serial.println("Client disconnected due to timeout.");
        client.stop();  // Putus koneksi klien
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("     C-Timeout     ");

        // Hentikan server selama 2 detik
        server.stop();
        Serial.println("Server stopped for 2 seconds.");
        delay(2000);  // Tunggu 2 detik sebelum memulai ulang server
        
        // Mulai ulang server
        server.begin();
        Serial.println("Server started again.");
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Waiting for data(t).");
        break;
      }
    }

    // Jika klien terputus
    if (!client.connected()) {
      Serial.println("Client disconnected.");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Waiting for data(s).");
    }
  }
}

// Fungsi untuk menampilkan data di LCD
void displayDataOnLCD(String data) {
  // Proses data sesuai format yang diinginkan
  int index = 0;
  String parts[6];
  while (data.indexOf('-') > 0 && index < 5) {
    int pos = data.indexOf('-');
    parts[index] = data.substring(0, pos);
    data = data.substring(pos + 1);
    index++;
  }
  parts[index] = data;  // Menyimpan bagian terakhir dari data


  // Tampilkan data pada LCD sesuai posisi dan tambahkan padding jika diperlukan
  lcd.setCursor(0, 0);
  lcd.print(parts[5]); // Nama dump truck

  lcd.setCursor(0, 1);
  lcd.print(parts[4]); // Payload

  lcd.setCursor(0, 2);
  lcd.print(parts[0]); // FL (Front Left)
 
  lcd.setCursor(10, 2);
  lcd.print(parts[1]); // FR (Front Right)

  lcd.setCursor(0, 3);
  lcd.print(parts[2]); // RL (Rear Left)

  lcd.setCursor(10, 3);
  lcd.print(parts[3]); // RR (Rear Right)

}
