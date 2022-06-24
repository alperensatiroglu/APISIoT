//https://miliohm.com/rc522-rfid-reader-with-nodemcu/ NodeMCU RFID-RC522 bağlantı ve kodları
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <TimeLib.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h" //!
#include "ESP8266WiFi.h"
#define API_KEY "AIzaSyAGNH1ngUDYzOJDWWCTiW5jh6CLhni3Edg"
#define FIREBASE_PROJECT_ID "apis-uygulama"
#define USER_EMAIL "apisarge@gmail.com"
#define USER_PASSWORD "apisapis"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

const char* ssid = "Alpis"; //Wi-fi kullanıcı adı(Telefonun hücresel ağı)
const char* password = "alpis2196"; //Wi-fi şifre
int status = WL_IDLE_STATUS;//?
WiFiClient  client;//?

String izinliler[] =       //İTU kartlarının id'leri uzun oldugu icin string olarak tanımladık
{
  "962273927",    //mavi sey
  "10212118473",  //Kenan Arslan
  "41104034",     //Serdar Bayraktar
  "1431071250",   //Alperen Şatıroğlu
  "161297110",    //Seyithan Karpuz
  "21710011214",  //Teoman Beyhun
  "13712617814",  //Ali Nabizadeh
  "207481249",    //Baran Ekşi
  "1112245164",   //Göksel Örsçekiç
  "2331689614",   //Murathan Bakır
  "751118573"     //Ufuk Köksal Yücedağ
};
int sayac = 0;  //ardisik kac kere giris yapıildi bilgisi

constexpr uint8_t RST_PIN = D3;
constexpr uint8_t SS_PIN = D4;

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

Servo myservo;
String tag;
String prvtag;

void FirestoreTokenStatusCallback(TokenInfo info){ //!
  //Serial.printf("Token Info: type = %s, status = %s\n", getTokenType(info).c_str(), getTokenStatus(info).c_str());
}

void firebaseInit(){
  config.api_key = API_KEY;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.token_status_callback = FirestoreTokenStatusCallback; //!

  Firebase.begin(&config, &auth);
}

void firestoreDataUpdate(){
  if(Firebase.ready()){
    String documentPath = "House/Room_2";

    FirebaseJson content;

    content.set("fields/temperature/doubleValue","10");
    //content.set("fields/humidity/doubleValue", String(humi).c_str());

    /*if(Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "temperature,humidity")){
      Serial.printf("ok\n%s\n\0n", fbdo.payload().c_str());
      return;
    }else{
      Serial.println(fbdo.errorReason());
    }*/

    if(Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())){
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      return;
    }else{
      Serial.println(fbdo.errorReason());
    }
  } else{
    Serial.println("fb not ready");
  }
}

void setup()
{
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  myservo.attach(D0, 500, 2400);
  myservo.write(100);
  //Wi-fi ya bağlanması
  WiFi.begin(ssid, password);
  // Serialportta wi-fi ya bağlanana kadar '.' yazdırıyor
  while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
  }
  //Yeni satır oluşturup WiFi connected yazdıktan sonra IP adresini yazdırıyor
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
  firebaseInit();
}

void loop() {
  if ( ! rfid.PICC_IsNewCardPresent())
    return;
  if (rfid.PICC_ReadCardSerial())
  {
    Serial.println("Okunuyor..."+ String(sayac) + " saniye gecti!");
    for (byte i = 0; i < 4; i++)
    {
      tag += rfid.uid.uidByte[i];
    }

    Serial.print("Kart ID: ");
    Serial.println(tag);
    bool flag = false;                                    //izinliler icindeyse true olur, degilse false devam der
    
    if (sayac==0)                                         //daha once izinli bir kart okunmamissa buraya girer
    {
      for (int i = 0; i < sizeof izinliler; i++)     //int size'i 4 oldugu icin 4'e bolduk sizeof(izinliler)/16
      {
         if (tag == izinliler[i])
        {
        sayac += 1;
        flag = true;                                      //eger okunan id izinliyse flag = true
        prvtag = tag;                                   //okunan kart hafizaya alınır
        Serial.println("Kapi Acildi!");
        Serial.println("----------------------");
        myservo.write(0);
        rfid.PICC_HaltA();
        
        firestoreDataUpdate();
        //delay(1000);
        break;
        }
      
        else
        {  
          if (!flag);             //izinliler icerisinde tag daha önce bulunmuşsa sayacı sifirlar
          {
          sayac = 0;
          }
          Serial.println("Kapi Kilitlendi!");
          Serial.println("----------------------");
          myservo.write(100);
          firestoreDataUpdate();
          //delay(1000);
        }
      }
    }
    else
    {
      if (tag == prvtag)              //kilitlemek icin okunan kartın aynı kart oldugunu kontrol eder
      {
        if (sayac >= 40)
        {
          Serial.println("Kapi Kilitlendi!");
          Serial.println("----------------------");
          myservo.write(100);
          sayac = 0;
          prvtag = "p";
          //delay(3000);    //kilitlendikten sonra 3 saniye boyunca okumayacak
          rfid.PICC_HaltA(); //kilitlendikten sonra kartı çekene kadar okumayacak
          firestoreDataUpdate();
          //delay(1000);
          }
        sayac += 1;
        delay(100);       //sayac 5 olana kadar her saniye test edecek
        
        }
      else                 //baska kart okunursa sayac sifirlanir ve kod yeniden baslar
      {
        sayac = 0;
        
        }
        
    }
  }
  tag = "";
  //rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
