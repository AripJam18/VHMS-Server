#include <Messages.h>
unsigned long previousMillis1 = 0;
const long interval1 = 5000;

const uint8_t ESCIT = 0x10;
const uint8_t STX = 2;
const uint8_t ETX = 3;
const uint8_t maxMsgLen = 100;

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);  // Initialize Serial2 if you're using it

  Serial.println(F("Starting..."));

  Messages::printMessage();
}

void loop() {
  Messages::sendMessage(Serial2, previousMillis1, interval1);
  getVHMS();
}

void data_V(const uint8_t* buffer, size_t len, bool chkOk) {
  dumpLine(buffer, len, chkOk);
  if (len == 20) {
    // Membuat satu string untuk menampung semua data
    String dataString = "";
    
    // Menambahkan Front-Left suspension pressure
    dataString += getPressureValue(buffer, 3);
    dataString += "-";  // Tambahkan tanda pemisah
    
    // Menambahkan Front-Right suspension pressure
    dataString += getPressureValue(buffer, 5);
    dataString += "-";  // Tambahkan tanda pemisah
    
    // Menambahkan Rear-Left suspension pressure
    dataString += getPressureValue(buffer, 7);
    dataString += "-";  // Tambahkan tanda pemisah
    
    // Menambahkan Rear-Right suspension pressure
    dataString += getPressureValue(buffer, 9);
    dataString += "-";  // Tambahkan tanda pemisah
    
    // Menambahkan Payload
    dataString += getPayloadValue(buffer, 13);
    dataString += "-";  // Tambahkan tanda pemisah

    // Menambahkan Identitas Atau Nama Unit(Dumptruck)
    dataString += F("HD78101KM");

    // Cetak semua data dalam satu string
    Serial.println(dataString);
  }
}

// Fungsi untuk mendapatkan nilai tekanan suspensi sebagai angka saja
String getPressureValue(const uint8_t* buffer, size_t pos) {
  uint16_t susP = buffer[pos] + (buffer[pos + 1] << 8);
  float pressureMPa = (susP * 0.1) * 0.0980665;  // Konversi ke MPa
  return String(pressureMPa, 2); // Format dengan 2 digit desimal
}

// Fungsi untuk mendapatkan nilai payload sebagai angka saja
String getPayloadValue(const uint8_t* buffer, size_t pos) {
  uint16_t pay = buffer[pos] + (buffer[pos + 1] << 8);
  float payloadValue = pay * 0.1; // Konversi payload
  return String(payloadValue, 1); // Format dengan 1 digit desimal
}

void dumpLine(const uint8_t* buffer, size_t len, bool chkOk) {
  Serial.println();
  if (len < 10) {
    Serial.write(' ');
  }
}

enum RxState {
  waitForSTX,
  collectUntilETX,
  waitForCRC,
};

void getVHMS() {
  static RxState rxs = waitForSTX;
  static bool nextCharLiteral = false;
  static uint8_t chkSum;
  static uint8_t bIndex;
  static uint8_t buffer[maxMsgLen + 1];
  if (Serial2.available()) {
    uint8_t inChar = Serial2.read();
    if (nextCharLiteral) {
      chkSum ^= inChar;
      buffer[bIndex++] = inChar;
      nextCharLiteral = false;
      return;
    } else if (inChar == ESCIT) {
      chkSum ^= inChar;
      nextCharLiteral = true;
      return;
    }
    switch (rxs) {
      case waitForSTX:
        if (inChar == STX) {
          bIndex = 0;
          chkSum = inChar;
          buffer[bIndex++] = inChar;
          rxs = collectUntilETX;
        }
        break;
      case collectUntilETX:
        chkSum ^= inChar;
        buffer[bIndex++] = inChar;
        if (inChar == ETX) {
          rxs = waitForCRC;
        }
        break;
      case waitForCRC:
        buffer[bIndex++] = inChar;

        data_V((const uint8_t*)(buffer + 1), bIndex - 3, inChar == chkSum);
        bIndex = 0;
        rxs = waitForSTX;
        break;
    }
  }
}


//output data yang diharapkan  : "3.25-3.50-2.75-3.00-10.5-HD78101KM"
