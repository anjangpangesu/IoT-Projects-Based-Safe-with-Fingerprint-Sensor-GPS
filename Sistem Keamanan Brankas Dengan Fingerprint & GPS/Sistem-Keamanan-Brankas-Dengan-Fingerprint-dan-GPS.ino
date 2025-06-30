/*
  Nama proyek     : Program sistem keamanan brankas menggunakan fingerprint, keypad, dan gps
  Microkontroller : ESP32-S3 DevitC 1
  DIbuat oleh     : Muhammad Alfian
*/

//Library yang digunakan
#include <TinyGPS++.h>              //Library untuk moudul GPS
#include <TimeLib.h>                //Library untuk mengatur waktu
#include <HardwareSerial.h>         //Library untuk pin serial (RX, TX)
#include <LiquidCrystal_I2C.h>      //Library untuk LCD 1602 + I2C
#include <Adafruit_Fingerprint.h>   //Library untuk modul fingerprint
#include <Keypad.h>                 //Library untuk keypad
#include "CTBot.h"                  //Library untuk bot telegram

//Setup nama wifi dan password
const char* ssid = "My home";        //Nama wifi, ini bisa diganti
const char* password = "Pangestu23"; //Password wifi, ini bisa diganti

//Setup bot telegram
CTBot myBot;   //Inisialisasi CTBot dengan myBot
String bottoken = "7508995696:AAGlIca0nWiJqvc031H0JWuSxuhh-EUcgjs"; //Input token bot telegram, , ini bisa diganti
String reply;  //Deklarasi reply sebagai string

//Setup keypad
#define ROWS  4   //definisikan rows 4
#define COLS  4   //definisikan cols 4

char keypressed;  //deklarasi keypressed sebagai char

char pinPass[]={'1','2','3','4'}; //Password default, bisa diubah dengan menyesuaikan arraynya
char c[4];                        //Array untuk password maksimal 4, ini bisa diganti jika ingin pinnya > 4
int ij;
int attempt;

char hexaKeys[ROWS][COLS] = { //Menjabarkan bentuk stamp keypad
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {40, 39, 38, 37};  //Pin baris yang digunakan pada stamp keypad
byte colPins[COLS] = {1, 2, 42, 41};    //Pin kolom yang digunakan pada stamp keypad

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

//Setup fingerprint
HardwareSerial fingerSerial(2);   //fingerSerial menggunakan serial 2 (RX2, TX2)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial); //Inisialisasi adafruit fingerprint dengan finger dan koneksikan dengan pin serial
uint8_t id;
int p;

//Setup GPS
HardwareSerial GPSSerial(1); //GPSSerial menggunakan serial 1 (RX1, TX1)
TinyGPSPlus gps;             //Inisialisasi tinygpsplus dengan gps

const int UTC_offset = 7;    //Zona Waktu WIB

//Setup lcd 1602 + i2c
LiquidCrystal_I2C lcd(0x27, 16, 2); //Alamat i2c yang digunakan

//Setup pin
#define Buzzer 11   //Buzzer ke Pin 11 esp32-s3
#define LedMerah 12 //Ledmerah ke pin 12 esp32-s3
#define LedHijau 13 //Ledhijau ke pin 13 esp32-s3
#define Doorlock 14 //Doorlock (Relay) ke pin 14 esp32-s3

void setup() {
  //Set baud rate untuk esp32-s3
  Serial.begin(115200); //Rekomendasi baud rate untuk esp32-s3
  
  //Set baud rate untuk gps dan koneksi pin
  GPSSerial.begin(9600, SERIAL_8N1, 18, 17); //Rekomendasi baud rate untuk gps, dan Pin RX1 gps ke pin 17 esp32-s3, pin TX1 gps ke pin 18 esp32-s3

  //Set baud rate untuk modul fingerprint
  finger.begin(57600);  //Rekomendasi baud rate untuk module fingerprint
  fingerSerial.begin(57600,SERIAL_8N1, 19, 20); // Pin RX2 gps ke pin 20 esp32-s3, pin TX2 gps ke pin 19 esp32-s3

  //Setup pin lcd 1602
  Wire.begin(10, 8);  //Pin SDA lcd ke pin 10 esp32-s3, pin SCL lcd ke pin 8 esp32-s3

  //Menyambungkan wifi dan token bot telegram
  myBot.wifiConnect(ssid, password);
	myBot.setTelegramToken(bottoken);

  //Cek koneksi internet
	if (myBot.testConnection()){
		Serial.println("\nKoneksi jaringan aman");
  }else{
		Serial.println("\nKoneksi jaringan tidak stabil");
  }

  //Setup mode pin, sebagai output atau input
  pinMode(LedMerah,OUTPUT);
  pinMode(LedHijau,OUTPUT);
  pinMode(Doorlock,OUTPUT);
  pinMode(Buzzer,OUTPUT);

  //Kondisi awal komponen
  digitalWrite(LedMerah,HIGH);  //Led merah nyala
  digitalWrite(LedHijau,LOW);   //Led hijau mati
  digitalWrite(Buzzer,LOW);     //Buzzer mati

  //Kondisi awal yang ditampilkan lcd
  lcd.init();                     //Inisialisasi lcd
  lcd.backlight();                //Layar lcd menjadi gelap dan menyala kembali dengan tulisan yang terdapat pada lcd.print
  lcd.setCursor(0,0);             //Tampilkan teks, paa baris 0 kolom 0
  lcd.print("     Halloo     ");  //Teks bisa diganti sesuai keinginan
  lcd.setCursor(0,1);             //Tampilkan teks, paa baris 0 kolom 1
  lcd.print(" Selamat Datang ");  //Teks bisa diganti sesuai keinginan
  delay(3000);                    //Delay disamping adalah 1000ms, konversi ke detik jadi 1 detik
  lcd.clear();                    //Hapus teks sebelumnya, agar tidak saling tindih antara teks sebelumnya dengan teks berikutnya
}

void loop() {
  //Inisilisasi tbmessage dengan msg
  TBMessage msg;

  keypressed = customKeypad.getKey();      //Membaca tipe button pada keypad
  if (keypressed != NO_KEY) {  // Jika ada tombol yang ditekan
    digitalWrite(Buzzer, HIGH);
    delay(20);
    digitalWrite(Buzzer, LOW);
  }
  if(keypressed == '*') {  //Tekan *, untuk masuk brankas dengan menginputkan pin dan sidik jari
    ij=0;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("    Silahkan    ");
    lcd.setCursor(0,1);
    lcd.print("Masukan Pin Pass");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Input Password :");
    getPassword();    //Memanggil fungsi getpassword
    if(ij==4) {       //Jika password benar, panggil fungsi pincorrect
      attempt=0;      //Attempt reset ke 0
      pinCorrect();
      delay(1000);
      lcd.clear();
    } else {          //Jika salah, panggil fungsi pinwrong
      attempt++;      //Jika salah attempt tambah 1
      pinWrong();
      if(attempt==3){ //Jika attempt = 3, panggil fungsi block
        block();
      }
    }
  }
  if(keypressed == '#') {   //Tekan #, untuk menambahkan sidik jari baru
    ij=0;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("    Silahkan    ");
    lcd.setCursor(0,1);
    lcd.print("Tambah Sidikjari");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Input Password :");
    getPassword();    //Memanggil fungsi getpassword
    if(ij==4) {       //Jika password benar, panggil fungsi pincorrect
      attempt=0;
      Enrolling();    //Memanggil fungsi enrolling
      delay(2000);
      lcd.clear();
    } else {          //Jika salah, panggil fungsi pinwrong
      attempt++;
      pinWrong();
      if(attempt==3){
        block();
      }
    }
  }
  //Tampilan lcd
  lcd.setCursor(0,0);
  lcd.print("Brankas Pin Pass");
  lcd.setCursor(0,1);
  lcd.print(" Dan Sidik Jari ");
  delay(2000);
}

//Fungsi getpassword untuk input dan ambil password yang sudah di set diatas
void getPassword(){
  for (int i=0 ; i<4 ; i++){
    c[i]= customKeypad.waitForKey();
    digitalWrite(Buzzer, HIGH);
    delay(20);
    digitalWrite(Buzzer, LOW);
    lcd.setCursor(i,1);
    lcd.print("*");
  }
  lcd.clear();
  for (int j=0 ; j<4 ; j++){  //Membandingkan pin yang dimasukkan dengan pin yang disimpan diboard yaitu 1234
    if(c[j]==pinPass[j]){     //Jika c = 4, maka pin sama dengan 4
      ij++;                    //Setiap kali karakternya benar, kita menambah ij hingga mencapai 4 yang berarti kodenya benar
    }   
  }
}

//Fungsi pinWrong, jika penginputan password salah maka fungsi yang terdapat di pinWrong dijalankan
void pinWrong(){
  reply="Password Yang Anda Masukan Salah"; //Pesan untuk dikirimkan ke telegram
  myBot.sendMessage(5357091291, reply);     //Mengirimkan pesan ke telegram dengan idchat = 5357091291, dan dengan pesan diatas.
  gpsTracking();                            //Panggil fungsi gpsTracking
  lcd.clear();                    
  lcd.setCursor(0,0);
  lcd.print(" Gagal Untuk Ke ");
  lcd.setCursor(0,1);
  lcd.print("Tahap Berikutnya");
  digitalWrite(Buzzer, HIGH);
  delay(120000UL);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Silahkan Ulangi ");
  lcd.setCursor(0,1);
  lcd.print("Isi Pin Yg Benar");
  digitalWrite(Buzzer, LOW);
  delay(3000);
  lcd.clear();
}

void block(){
  reply="Anda Telah Menginputkan Pin Yang Salah Sebanyak 3 Kali, Brankas Terkunci Selama 5 Menit"; //Pesan untuk dikirimkan ke telegram
  myBot.sendMessage(5357091291, reply);     //Mengirimkan pesan ke telegram dengan idchat = 5357091291, dan dengan pesan diatas.
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  3 Kali Input  ");
  lcd.setCursor(0,1);
  lcd.print(" Pass Yang Salah");
  delay(3000);
  lcd.clear();                    
  lcd.setCursor(0,0);
  lcd.print(" Silahkan Tunggu");
  lcd.setCursor(0,1);
  lcd.print(" Selama 5 Menit");
  digitalWrite(Buzzer, HIGH);
  delay(300000UL);
  digitalWrite(Buzzer, LOW);
  reply="5 Menit Sudah Berlalu, Silahkan Input Password Yang Benar!"; //Pesan untuk dikirimkan ke telegram
  myBot.sendMessage(5357091291, reply);     //Mengirimkan pesan ke telegram dengan idchat = 5357091291, dan dengan pesan diatas.
  lcd.clear();                    
  lcd.setCursor(0,0);
  lcd.print("   Waktu Habis  ");
  lcd.setCursor(0,1);
  lcd.print("Input Ulang Pass");
  delay(3000);
  lcd.clear();
}

//Fungsi pinCorrect, jika penginputan password benar maka fungsi yang terdapat di pinCorrect dijalankan
void pinCorrect(){
  Serial.println("Password Benar");
  reply="Password Yang Anda Masukan Benar";
  myBot.sendMessage(5357091291,reply);
  gpsTracking();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" Password Benar ");
  digitalWrite(LedHijau, LOW);
  digitalWrite(LedMerah, HIGH);
  digitalWrite(Buzzer, HIGH);
  delay(200);
  digitalWrite(Buzzer, LOW);
  delay(200);
  digitalWrite(Buzzer, HIGH);
  delay(200);
  digitalWrite(Buzzer, LOW);
  delay(200);
  digitalWrite(Buzzer, HIGH);
  delay(200);
  digitalWrite(Buzzer, LOW);
  delay(3000);
  digitalWrite(LedHijau, HIGH);
  digitalWrite(LedMerah, LOW);
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Silahkan Lakukan");
  lcd.setCursor(0,1);
  lcd.print(" Scan SidikJari ");
  delay(3000);
  getFingerprintIDez();                 //Panggil fungsi getFingerprintIDez
}

////Fungsi fingerWrong, jika scan sidik jari salah maka fungsi yang terdapat di fingerWrong dijalankan
void fingerWrong(){
  Serial.println("Sidik Jari Anda Salah");
  reply="Sidik Jari Anda Salah Atau Tidak Dikenali";
  myBot.sendMessage(5357091291,reply);
  gpsTracking();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Sidik Jari Anda");
  lcd.setCursor(0,1);
  lcd.print("     Salah     ");
  digitalWrite(Buzzer, HIGH);
  delay(120000UL);
  digitalWrite(Buzzer, LOW);
}

////Fungsi fingerWrong, untuk mengirimkan sinyal gps dengan titik lokasi, tanggal, dan waktu pada saat input password benar & salah dan pada saat scan sidik jari benar & salah
void gpsTracking(){
  while (GPSSerial.available()) {                       //Jika gpsserial tersedia, maka jalankan perintah diibawahnya dengan berulang
    if (gps.encode(GPSSerial.read())){                  //Jika data GPS valid, maka jalankan perintah selanjutnya
      //Mengambil informasi tanggal (Year, Month, Day) dan waktu (Hour, Minute, Second) dari data GPS yang telah diolah.
      byte Year = gps.date.year();
      byte Month = gps.date.month();
      byte Day = gps.date.day();
      byte Hour = gps.time.hour();
      byte Minute = gps.time.minute();
      byte Second = gps.time.second();
      //Sinkronisasi waktu perangkat dengan waktu GPS
      setTime(Hour, Minute, Second, Day, Month, Year);
      //Mengoreksi waktu dari zona waktu GPS ke zona waktu lokal
      adjustTime(UTC_offset * SECS_PER_HOUR);
    }
  }
  //Jika data GPS sudah cukup lengkap untuk digunakan, maka jalankan perintah dibawahnya
  if (gps.charsProcessed() >= 0) {
    //Mengambil data lintang (latitude) dan bujur (longitude) lokasi saat ini
    float currentLat = gps.location.lat();
    float currentLng = gps.location.lng();
    //Mengirimkan URL Google Maps berdasarkan koordinat lokasi ke bot telegram
    reply="Location: https://maps.google.com/?q=" + String(currentLat, 6) + "," + String(currentLng, 8);
    myBot.sendMessage(5357091291,reply);
    //Mengirimkan data tanggal dan waktu ke bot telegram
    reply="Tanggal: " + String(day()) + "/" + String(month()) + "/" + String(year()) + ", Pukul: " + String(hour()) + ":" + String(minute()) + ":" + String(second());
    myBot.sendMessage(5357091291,reply);
  }
}

//Fungsi enrolling, digunakan untuk menjalankan perintah enroll (tambah sidik jari)
void Enrolling()  {
  keypressed = NULL;         //Keypad kondisi nol (tidak terdapat nilai), yang bisa digunakan 0-9
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   Melakukan   ");
  lcd.setCursor(0,1);
  lcd.print("  Enroll Baru  ");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Input ID Baru  ");
  id = readnumber();          //Simpan fungsi readnumber ke id
  if (id == 0) {              //Jika ID sama dengan 0, kembali
     return;
  }
  while (!  getFingerprintEnroll() ); //Jika id sudah diinput dan benar, maka jalankan fungsi getFingerprintEnroll
}

//Fungsi untuk melakukan enroll sidik jari, JANGAN EDIT BAGIAN INI KECULI KOMENTAR
uint8_t getFingerprintEnroll() {
  int p = -1;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Enroll Berhasil");
  lcd.setCursor(0,1);
  lcd.print("Enroll ID : ");
  lcd.setCursor(10,1);
  lcd.print(id);
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Silahkan Tempel");
  lcd.setCursor(0,1);
  lcd.print("Sidik Jari Anda");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }
  // OK success!
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      break;
    case FINGERPRINT_IMAGEMESS:
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      return p;
    case FINGERPRINT_FEATUREFAIL:
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      return p;
    default:
      return p;
  }
  digitalWrite(Buzzer, HIGH);
  delay(200);
  digitalWrite(Buzzer, LOW);
  delay(200);
  digitalWrite(Buzzer, HIGH);
  delay(200);
  digitalWrite(Buzzer, LOW);
  delay(200);
  digitalWrite(Buzzer, HIGH);
  delay(200);
  digitalWrite(Buzzer, LOW);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("    Silahkan   ");
  lcd.setCursor(0,1);
  lcd.print(" Lepaskan Jari "); //After getting the first template successfully
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  p = -1;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Letakan Kembali"); //We launch the same thing another time to get a second template of the same finger
  lcd.setCursor(0,1);
  lcd.print("Sidik Jari Anda"); 
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }
  // OK success!
  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      break;
    case FINGERPRINT_IMAGEMESS:
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      return p;
    case FINGERPRINT_FEATUREFAIL:
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      return p;
    default:
      return p;
  }
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    return p;
  } else {
    return p;
  }   
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    digitalWrite(Buzzer, HIGH);
    delay(200);
    digitalWrite(Buzzer, LOW);
    delay(200);
    digitalWrite(Buzzer, HIGH);
    delay(200);
    digitalWrite(Buzzer, LOW);
    delay(200);
    digitalWrite(Buzzer, HIGH);
    delay(200);
    digitalWrite(Buzzer, LOW);
    lcd.clear();                //if both images are gotten without problem we store the template as the Id we entred
    lcd.setCursor(0,0);
    lcd.print("Tersimpan Untuk");    //Print a message after storing and showing the ID where it's stored
    lcd.setCursor(0,1);
    lcd.print("ID : ");
    lcd.setCursor(6,1);
    lcd.print(id);
    delay(3000);
    lcd.clear();
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    return p;
  } else {
    return p;
  }   
}

// Fungsi ini mendapatkan nomor ID sebagai format 3 digit seperti 001 untuk ID 1
// Dan kembalikan nomor tersebut ke fungsi pendaftaran
uint8_t readnumber(void) {
  uint8_t num = 0;
   while (num == 0) {
    char keey = customKeypad.waitForKey();
    digitalWrite(Buzzer, HIGH);
    delay(20);
    digitalWrite(Buzzer, LOW);
    int  num1 = keey-48;
         lcd.setCursor(0,1);
         lcd.print(num1);
         keey = customKeypad.waitForKey();
         digitalWrite(Buzzer, HIGH);
         delay(20);
         digitalWrite(Buzzer, LOW);
    int  num2 = keey-48;
         lcd.setCursor(1,1);
         lcd.print(num2);
         keey = customKeypad.waitForKey();
         digitalWrite(Buzzer, HIGH);
         delay(20);
         digitalWrite(Buzzer, LOW);
    int  num3 = keey-48;
         lcd.setCursor(2,1);
         lcd.print(num3);
         digitalWrite(Buzzer, HIGH);
         delay(20);
         digitalWrite(Buzzer, LOW);
         delay(1000);
         num=(num1*100)+(num2*10)+num3;
         keey=NO_KEY;
  }
  return num;
}

//Fungsi getFingerprintIDez digunakan untuk menunggu sidik jari, memindainya dan memberikan akses jika dikenali
int getFingerprintIDez() {
  uint8_t p = finger.getImage();       //Memindai gambar
  if (p != FINGERPRINT_OK)  return -1;  

  p = finger.image2Tz();               //Mengkonversi
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();     //Mencari kecocokan pada memori
  if (p != FINGERPRINT_OK){          //jika pencarian gagal, maka sidik jari tidak terdaftar dan panggil fungsi fingerWrong
    fingerWrong();                   //Fungsi fingerWrong
    return -1;
  }
  //Perintah yang dijalankan jika sidik jari dikenali
  reply="Scan Sidik Jari Anda Benar";
  myBot.sendMessage(5357091291,reply);
  gpsTracking();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" Selamat Datang ");
  lcd.setCursor(0,1);
  lcd.print(" ID : ");
  lcd.setCursor(7,1);
  lcd.print(finger.fingerID);       //ID dari fingerprint yang sudah terdaftar
  Opendoorlock();                   //Jalankan fungsi Opendoorlock
  delay(8000);
  Closedoorlock();                  //Jalankan fungsi Closedoorlock
  delay(1000);
  lcd.clear();
  lcd.init();
  lcd.backlight();
  return finger.fingerID;           //Kembalikan ID dari fingerprint yang sudah terdaftar
}

//Fungsi Opendoorlock untuk menjalankan perintah pada saat brankas akan dibuka
void Opendoorlock (){
  reply="Brankas Terbuka Selama 10 Menit";
  myBot.sendMessage(5357091291,reply);
  lcd.clear();
  lcd.print("Brankas Terbuka!");
  lcd.setCursor(0,1);
  lcd.print("Selama 10 Menit!");
  digitalWrite(LedHijau, HIGH);
  digitalWrite(LedMerah, LOW);
  digitalWrite(Doorlock, HIGH);  //Relay aktif
  digitalWrite(Buzzer, HIGH);
  delay(200);
  digitalWrite(Buzzer, LOW);
  delay(200);
  digitalWrite(Buzzer, HIGH);
  delay(200);
  digitalWrite(Buzzer, LOW);
  delay(200);
  digitalWrite(Buzzer, HIGH);
  delay(200);
  digitalWrite(Buzzer, LOW);
  delay(540000UL);
  reply="Waktu Anda Tersisa 1 Menit Lagi, Harap Segera Tutup Brankasnya";
  myBot.sendMessage(5357091291,reply);
  lcd.clear();
  lcd.print(" Waktu Tersisa  ");
  lcd.setCursor(0,1);
  lcd.print("Satu Menit Lagi ");
  digitalWrite(Buzzer, HIGH);
  delay(60000UL);
  digitalWrite(Buzzer, LOW);
  reply="Waktu Anda Habis! Brankas Tertutup";
  myBot.sendMessage(5357091291,reply);
  lcd.clear();
  lcd.print("  Waktu Habis!  ");
  lcd.setCursor(0,1);
  lcd.print("Brankas Tertutup");
}

//Fungsi Closedoorlock untuk menjalankan perintah pada saat brankas akan ditutup
void Closedoorlock(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Brankas Tertutup");
  lcd.setCursor(0,1); 
  lcd.print("     Kembali    ");
  digitalWrite(LedHijau, LOW);
  digitalWrite(LedMerah, HIGH);
  digitalWrite(Doorlock, LOW); //Relay mati
}


