#include "esp_camera.h"
#include <WiFi.h>
#include "Arduino.h" // Diperlukan untuk HardwareSerial jika belum termasuk secara implisit

// Pilih model kamera
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

const char* ssid = "Project"; // Ganti dengan SSID WiFi Anda
const char* password = "123456789"; // Ganti dengan password WiFi Anda

void startCameraServer(); // Deklarasi fungsi dari app_httpd.cpp

// Deklarasi extern untuk variabel global dari app_httpd.cpp
// Ini memungkinkan file .ino ini untuk membaca dan mengubah nilainya.
extern int8_t is_enrolling;
extern int8_t detection_enabled;
extern int8_t recognition_enabled;

// Deklarasi extern untuk Serial2 yang digunakan di app_httpd.cpp
// Pastikan pin RX/TX (16/17) sesuai dengan wiring fisik Anda ke Arduino.
// TX ESP32-CAM (GPIO17) ke RX Arduino
// RX ESP32-CAM (GPIO16) ke TX Arduino
extern HardwareSerial Serial2;


void setup() {
  Serial.begin(115200); // Baud rate untuk Serial Monitor (Debug)
  Serial.setDebugOutput(true);
  Serial.println();

  // Inisialisasi Serial2 untuk komunikasi dengan Arduino
  // Baud rate harus sama dengan yang diset di Arduino Anda
  // Pin 16 (RX2) dan 17 (TX2) adalah default untuk UART2 pada ESP32
  // Pastikan Anda menghubungkan GPIO17 (TX ESP32-CAM) ke RX Arduino
  // dan GPIO16 (RX ESP32-CAM) ke TX Arduino
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("Serial2 initialized for Arduino communication (TX: GPIO17, RX: GPIO16).");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Inisialisasi kamera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    Serial2.println("CAM_ERR"); // Kirim sinyal error ke Arduino
    return;
  }
  Serial.println("Camera initialized.");

  sensor_t * s = esp_camera_sensor_get();
  // Mengatur ke QVGA untuk performa yang lebih baik saat face recognition
  // (Penting: UXGA/SVGA terlalu besar untuk Face Recognition real-time di ESP32)
  s->set_framesize(s, FRAMESIZE_QVGA);
  Serial.println("Framesize set to QVGA for Face AI.");

  // --- MODIFIKASI DIMULAI DI SINI ---

  // Mengatur kamera agar diflip secara vertikal (vflip)
  // Nilai 1 untuk vflip aktif, 0 untuk non-aktif
  s->set_vflip(s, 1);
  Serial.println("Camera VFLIP enabled.");

  // --- MODIFIKASI SELESAI DI SINI ---

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  startCameraServer(); // Memulai server HTTP untuk streaming dan kontrol

  // --- MODIFIKASI DIMULAI DI SINI ---

  // Aktifkan Face Detection dan Face Recognition secara default saat booting
  detection_enabled = 1;
  recognition_enabled = 1;
  is_enrolling = 0; // Pastikan enrollment non-aktif saat booting

  Serial.println("ESP32-CAM initial mode: Face Detection & Recognition enabled.");
  Serial2.println("CAM_READY"); // Kirim sinyal ke Arduino bahwa CAM siap

  // --- MODIFIKASI SELESAI DI SINI ---
}

void loop() {
  // Loop ini kosong karena semua pekerjaan utama (streaming, deteksi wajah)
  // ditangani oleh server HTTP dan proses background di app_httpd.cpp.
  // Anda bisa menambahkan logika di sini jika ada tugas berulang yang perlu dijalankan
  // di luar konteks server web, misalnya membaca sensor eksternal.
  delay(10000); // Penundaan agar loop tidak berjalan terlalu cepat jika tidak ada kode lain.
}
