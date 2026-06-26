# Smart Door Access Control System

**Secure Door Access Management** menggunakan Face Recognition dan RFID Technology.

Sistem keamanan pintu laboratorium berbasis multi-factor authentication yang menggabungkan pengenalan wajah dan kartu RFID untuk meningkatkan keamanan dan memantau akses masuk keluar.

## ✨ Fitur Utama

- **Face Recognition**: Deteksi dan verifikasi wajah menggunakan algoritma matematika
- **RFID Integration**: Membaca kartu RFID sebagai autentikasi kedua
- **Multi-Factor Authentication**: Kombinasi wajah + RFID untuk keamanan maksimal
- **Real-time Logging**: Mencatat setiap akses masuk/keluar beserta waktu dan identitas
- **Access Control**: Otomatis membuka/kunci pintu berdasarkan verifikasi

## 📋 Cara Kerja Sistem

1. Pengguna mendekatkan kartu RFID ke reader
2. Sistem membaca UID kartu dan mencocokkan dengan database
3. Jika RFID valid, kamera akan mengambil gambar wajah
4. Sistem melakukan face recognition
5. Jika **keduanya cocok**, pintu akan terbuka secara otomatis
6. Semua aktivitas dicatat ke database lokal