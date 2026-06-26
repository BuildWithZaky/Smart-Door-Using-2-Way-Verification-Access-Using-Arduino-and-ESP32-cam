#include <EEPROM.h>     
#include <Wire.h>       
#include <Adafruit_PN532.h>  
#include <LiquidCrystal_I2C.h>

// Inisialisasi LCD I2C (alamat default 0x27 untuk 16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define relay 6 // Pin untuk relay yang mengontrol solenoid

// Inisialisasi PN532 untuk I2C tanpa IRQ atau RESET
Adafruit_PN532 nfc(-1, -1);

boolean match = false;          
boolean programMode = false;  
boolean replaceMaster = false;

uint8_t successRead;    
byte storedCard[4];   
byte readCard[4];   
byte masterCard[4];   
byte lastReadCard[4]; // Untuk menyimpan UID kartu terakhir yang dibaca
boolean cardPresent = false; // Untuk melacak apakah kartu masih ada

// Untuk logika dua faktor (opsional)
boolean rfidValid = false;
unsigned long rfidTime = 0;
const unsigned long timeout = 10000; // 10 detik timeout

void setup() { 
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH); // Relay mati (solenoid terkunci)
  Serial.begin(9600); // Baud rate untuk komunikasi dengan ESP32-CAM
  
  Wire.begin();          
  nfc.begin();   

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Palang Pintu");
  lcd.setCursor(0, 1);
  lcd.print("RFID & Wajah");
  delay(2000);
  lcd.clear();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println(F("Modul PN532 tidak ditemukan"));
    lcd.setCursor(0, 0);
    lcd.print("PN532 Tidak");
    lcd.setCursor(0, 1);
    lcd.print("Ditemukan");
    while (true); // Hentikan program
  }
  Serial.print(F("Chip PN5")); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print(F("Firmware ver. ")); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  nfc.SAMConfig(); // Konfigurasi PN532 untuk baca tag RFID
  Serial.println(F("Palang Pintu Otomatis dengan RFID dan Deteksi Wajah"));   

  if (EEPROM.read(1) != 143) {
    Serial.println(F("Tidak Ada Master Card Tersimpan"));
    Serial.println(F("Scan Kartu RFID Untuk Dijadikan Master Card"));
    lcd.setCursor(0, 0);
    lcd.print("Scan Master");
    lcd.setCursor(0, 1);
    lcd.print("Card RFID");
    do {
      successRead = getID();            
      delay(400);
    }
    while (!successRead);                  
    for (uint8_t j = 0; j < 4; j++) {        
      EEPROM.write(2 + j, readCard[j]);  
    }
    EEPROM.write(1, 143);                  
    Serial.println(F("Master Card Tersimpan"));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Master Card");
    lcd.setCursor(0, 1);
    lcd.print("Tersimpan");
    delay(2000);
    lcd.clear();
  }
  Serial.println(F("-------------------"));
  Serial.println(F("Kartu RFID Master Card"));
  for (uint8_t i = 0; i < 4; i++) {         
    masterCard[i] = EEPROM.read(2 + i);    
    Serial.print(masterCard[i], HEX);
  }
  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Sudah Siap"));
  Serial.println(F("Menunggu Kartu RFID atau Sinyal Wajah"));
  lcd.setCursor(0, 0);
  lcd.print("Siap Scan");
  lcd.setCursor(0, 1);
  lcd.print("RFID/Wajah");
  cycleLcd();    
}

void loop() {   
  // Cek RFID
  successRead = getID();  
  if (successRead) {
    if (programMode) {
      cycleLcd();              
      if (isMaster(readCard)) { 
        Serial.println(F("Master Card Terbaca"));
        Serial.println(F("Keluar Program Mode"));
        Serial.println(F("-----------------------------"));
        programMode = false;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Keluar");
        lcd.setCursor(0, 1);
        lcd.print("Program Mode");
        delay(2000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Siap Scan");
        lcd.setCursor(0, 1);
        lcd.print("RFID/Wajah");
        return;
      }
      else {
        if (findID(readCard)) { 
          Serial.println(F("Hapus Kartu RFID"));
          deleteID(readCard);
          Serial.println("-----------------------------");
          Serial.println(F("Scan Kartu RFID Tambahkan Atau Hapus Ke EEPROM"));
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Kartu RFID");
          lcd.setCursor(0, 1);
          lcd.print("Dihapus");
          delay(2000);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Scan Tambah");
          lcd.setCursor(0, 1);
          lcd.print("Atau Hapus");
        }
        else {                    
          Serial.println(F("Tambahkan Kartu RFID"));
          writeID(readCard);
          Serial.println(F("-----------------------------"));
          Serial.println(F("Scan Kartu RFID Tambahkan Atau Hapus Ke EEPROM"));
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Kartu RFID");
          lcd.setCursor(0, 1);
          lcd.print("Ditambahkan");
          delay(2000);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Scan Tambah");
          lcd.setCursor(0, 1);
          lcd.print("Atau Hapus");
        }
      }
    }
    else {
      if (isMaster(readCard)) {   
        programMode = true;
        Serial.println(F("Masuk Ke Program Mode"));
        uint8_t count = EEPROM.read(0);   
        Serial.print(F("Ada "));     
        Serial.print(count);
        Serial.print(F(" Data Di EEPROM"));
        Serial.println("");
        Serial.println(F("Scan Kartu RFID Tambahkan Atau Hapus Ke EEPROM"));
        Serial.println(F("Scan Master Card Lagi Untuk Keluar dari Program Mode"));
        Serial.println(F("-----------------------------"));
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Program Mode");
        lcd.setCursor(0, 1);
        lcd.print("Scan Tambah/Hapus");
        delay(2000);
      }
      else {
        if (findID(readCard)) {
          Serial.println(F("Selamat Datang, RFID Valid"));
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("RFID Valid");
          lcd.setCursor(0, 1);
          lcd.print("Tunggu Wajah");
          // Untuk mode sederhana: langsung buka pintu
          diterima(5000);
          // Untuk mode dua faktor (uncomment baris berikut, comment diterima di atas):
          // rfidValid = true;
          // rfidTime = millis();
          // Serial.println(F("Menunggu Verifikasi Wajah"));
        }
        else {      
          Serial.println(F("Anda Tidak Bisa Masuk"));
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Akses Ditolak");
          lcd.setCursor(0, 1);
          lcd.print("RFID Tidak Valid");
          ditolak();
        }
      }
    }
  }

  // Cek perintah serial dari ESP32-CAM
  if (Serial.available() > 0) {
    char command = Serial.read();
    // Untuk mode sederhana: buka pintu jika wajah dikenali
    if (command == 'F') {
      Serial.println(F("Wajah Dikenali, Membuka Pintu"));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wajah Dikenali");
      lcd.setCursor(0, 1);
      lcd.print("Pintu Dibuka");
      diterima(5000);
    }
    // Untuk mode dua faktor (uncomment baris berikut, comment if di atas):
    if (command == 'F' && rfidValid && (millis() - rfidTime < timeout)) {
      Serial.println(F("Wajah dan RFID Valid, Membuka Pintu"));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("RFID & Wajah");
      lcd.setCursor(0, 1);
      lcd.print("Valid, Dibuka");
      diterima(5000);
      rfidValid = false; // Reset status
    }
  }

  // Reset rfidValid setelah timeout (hanya untuk mode dua faktor)
  if (rfidValid && (millis() - rfidTime >= timeout)) {
    rfidValid = false;
    Serial.println(F("Timeout Verifikasi Wajah"));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timeout");
    lcd.setCursor(0, 1);
    lcd.print("Verifikasi Wajah");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Siap Scan");
    lcd.setCursor(0, 1);
    lcd.print("RFID/Wajah");
  }
  

  delay(100); // Jeda untuk mencegah pembacaan berulang
}

// Program saat kartu RFID atau wajah diterima
void diterima(uint16_t setDelay) {
  digitalWrite(relay, LOW); // Aktifkan relay (buka solenoid)
  delay(setDelay); // Tahan solenoid terbuka selama setDelay ms
  digitalWrite(relay, HIGH); // Nonaktifkan relay (kunci solenoid)
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Siap Scan");
  lcd.setCursor(0, 1);
  lcd.print("RFID/Wajah");
}

// Program saat kartu RFID ditolak
void ditolak() {
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Siap Scan");
  lcd.setCursor(0, 1);
  lcd.print("RFID/Wajah");
}

// Program untuk baca ID kartu RFID
uint8_t getID() { 
  uint8_t success;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer untuk simpan UID
  uint8_t uidLength; // Panjang UID

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100); // Timeout 100ms
  
  if (success && uidLength == 4) { // Pastikan UID 4 byte
    // Cek apakah UID sama dengan yang terakhir dibaca
    boolean sameCard = true;
    for (uint8_t i = 0; i < 4; i++) {
      if (uid[i] != lastReadCard[i]) {
        sameCard = false;
        break;
      }
    }
    if (sameCard && cardPresent) {
      return 0; // Abaikan jika kartu sama dan masih ada
    }
    
    // Simpan UID baru
    for (uint8_t i = 0; i < 4; i++) {  
      readCard[i] = uid[i];
      lastReadCard[i] = uid[i];
    }
    cardPresent = true;
    
    Serial.println(F("Scan ID Kartu RFID:"));
    for (uint8_t i = 0; i < 4; i++) {  
      Serial.print(readCard[i], HEX);
    }
    Serial.println("");
    return 1;
  }
  else {
    cardPresent = false; // Kartu tidak ada
    return 0;
  }
}

// Indikator masuk ke mode program
void cycleLcd() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Program Mode");
  lcd.setCursor(0, 1);
  lcd.print("Aktif");
  delay(200);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan Tambah");
  lcd.setCursor(0, 1);
  lcd.print("Atau Hapus");
  delay(200);
}

// Program baca ID kartu RFID dari EEPROM
void readID(uint8_t number) {
  uint8_t start = (number * 4) + 2;    
  for (uint8_t i = 0; i < 4; i++) {     
    storedCard[i] = EEPROM.read(start + i);  
  }
}

// Program tambah ID kartu RFID ke EEPROM
void writeID(byte a[]) {
  if (!findID(a)) {     
    uint8_t num = EEPROM.read(0);     
    uint8_t start = (num * 4) + 6;  
    num++;                
    EEPROM.write(0, num);    
    for (uint8_t j = 0; j < 4; j++) {   
      EEPROM.write(start + j, a[j]);  
    }
    successWrite();
    Serial.println(F("Berhasil Menambahkan ID Kartu RFID ke EEPROM"));
  }
  else {
    failedWrite();
    Serial.println(F("Gagal Memasukkan ID Kartu RFID ke EEPROM"));
  }
}

void deleteID(byte a[]) {
  if (!findID(a)) {     
    failedWrite();     
    Serial.println(F("Gagal Memasukkan ID Kartu RFID ke EEPROM"));
  }
  else {
    uint8_t num = EEPROM.read(0);  
    uint8_t slot;       
    uint8_t start;      
    uint8_t looping;    
    uint8_t j;
    uint8_t count = EEPROM.read(0); 
    slot = findIDSLOT(a);   
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      
    EEPROM.write(0, num);   
    for (j = 0; j < looping; j++) {         
      EEPROM.write(start + j, EEPROM.read(start + 4 + j));   
    }
    for (uint8_t k = 0; k < 4; k++) {        
      EEPROM.write(start + j + k, 0);
    }
    successDelete();
    Serial.println(F("Berhasil Menghapus ID Kartu RFID dari EEPROM"));
  }
}

boolean checkTwo(byte a[], byte b[]) {
  if (a[0] != 0)      
    match = true;       
  for (uint8_t k = 0; k < 4; k++) {   
    if (a[k] != b[k])     
      match = false;
  }
  if (match) {      
    return true;      
  }
  else {
    return false;       
  }
}

uint8_t findIDSLOT(byte find[]) {
  uint8_t count = EEPROM.read(0);       
  for (uint8_t i = 1; i <= count; i++) {    
    readID(i);                
    if (checkTwo(find, storedCard)) {   
      return i;         
      break;          
    }
  }
}

boolean findID(byte find[]) {
  uint8_t count = EEPROM.read(0);     
  for (uint8_t i = 1; i <= count; i++) {    
    readID(i);          
    if (checkTwo(find, storedCard)) {   
      return true;
      break;  
    }
  }
  return false;
}

void successWrite() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Berhasil");
  lcd.setCursor(0, 1);
  lcd.print("Tulis EEPROM");
  delay(2000);
}

void failedWrite() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gagal Tulis");
  lcd.setCursor(0, 1);
  lcd.print("EEPROM");
  delay(2000);
}

void successDelete() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Berhasil");
  lcd.setCursor(0, 1);
  lcd.print("Hapus EEPROM");
  delay(2000);
}

boolean isMaster(byte test[]) {
  if (checkTwo(test, masterCard))
    return true;
  else
    return false;
}

/* Jika pin RX/TX default sudah digunakan, gunakan SoftwareSerial:
#include <SoftwareSerial.h>
SoftwareSerial espSerial(10, 11); // RX, TX
Ganti Serial dengan espSerial di setup() dan loop() untuk komunikasi dengan ESP32-CAM.
*/
