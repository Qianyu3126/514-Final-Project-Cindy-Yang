/************ Firebase config ************/
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <esp_sleep.h>

/************ HC-SR04 ************/
#define TRIG_PIN 4
#define ECHO_PIN 5

/************ WiFi ************/
const char* ssid = "UW MPSK";
const char* password = "64vxLzbrQ4nVgt6x";

/************ Firebase Auth ************/
UserAuth user_auth(
  "AIzaSyC3SdOtm3aToeo2sE7QDjg323GliVnLpL4", 
  "loomie2014@gmail.com",
  "1112126yangqy"
);

/************ Firebase objects ************/
FirebaseApp app;
WiFiClientSecure ssl_client;
AsyncClientClass async_client(ssl_client);
RealtimeDatabase Database;

/************ Power Policy ************/
#define DIST_THRESHOLD_CM 50        
#define ACTIVE_SLEEP_SEC 5         
#define IDLE_SLEEP_SEC 20           

RTC_DATA_ATTR bool lastDetected = false;  

/************ Ultrasonic ************/
long readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return duration == 0 ? -1 : duration * 0.034 / 2;
}

/************ setup ************/
void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  /************ Read sensor ************/
  long distance = readUltrasonic();
  bool detected = (distance > 0 && distance <= DIST_THRESHOLD_CM);

  Serial.printf("Distance: %ld cm | Object: %s\n",
                distance,
                detected ? "YES" : "NO");

  /************ WiFi ************/
  WiFi.begin(ssid, password);
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - wifiStart < 8000) {
    delay(200);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");

    /************ Firebase ************/
    ssl_client.setInsecure();
    initializeApp(async_client, app, getAuth(user_auth));
    app.getApp(Database);
    Database.url("https://cindyyang514lab5-default-rtdb.firebaseio.com/");

    unsigned long fbStart = millis();
    while (!app.ready() && millis() - fbStart < 5000) {
      app.loop();
      delay(50);
    }

    if (app.ready()) {
      Serial.println("Firebase ready");

      if (detected != lastDetected || detected) {
        Database.set(async_client,
                     "/lab5/current_distance",
                     distance);

        Database.push(async_client,
                      "/lab5/history",
                      distance);

        Database.set(async_client,
                     "/lab5/objectDetected",
                     detected);

        Serial.println("Firebase updated");
      }
    } else {
      Serial.println("Firebase not ready, skip upload");
    }
  } else {
    Serial.println("WiFi failed");
  }

  lastDetected = detected;

  /************ Decide sleep ************/
  uint32_t sleepSec = detected ? ACTIVE_SLEEP_SEC : IDLE_SLEEP_SEC;

  Serial.printf("Deep sleeping for %lu seconds\n", sleepSec);

  esp_sleep_enable_timer_wakeup(
    (uint64_t)sleepSec * 1000000ULL
  );
  esp_deep_sleep_start();
}

/************ loop ************/
void loop() {
}
