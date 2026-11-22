/* =============================================================================
 *  PROJET RED/BLUE - "TempRegulator Supermarket Attack"
 *  Fichier  : tab5_victim.ino
 *  Rôle     : AFFICHEUR SUPERVISION (M5Stack Tab5) = pupitre du rayon frais
 *             -> affiche température, humidité, état dégivrage, notifications.
 *             -> c'est le programme que l'ÉQUIPE BLEUE va DURCIR
 *                (Login/PW, MQTT sécurisé TLS, filtrage MAC côté routeur).
 *
 *  Librairies : M5Unified, PubSubClient, ArduinoJson.
 *  Note Tab5  : WiFi via co-processeur ESP32-C6 (pile WiFi standard Arduino).
 *
 *  Les points à durcir sont marqués  // [DURCISSEMENT BLEU].
 * ============================================================================= */

#include <M5Unified.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/* ----------------------------- CONFIG RÉSEAU -------------------------------- */
const char* WIFI_SSID = "RAYON_FRAIS";
const char* WIFI_PASS = "supermarket2026";
const char* MQTT_HOST = "192.168.10.10";     // PC Node-RED / Mosquitto
uint16_t    MQTT_PORT = 1883;                // [DURCISSEMENT BLEU] -> 8883 TLS
const char* MQTT_USER = "";                  // [DURCISSEMENT BLEU] -> compte "hmi"
const char* MQTT_PASS = "";
const char* CLIENT_ID = "hmi-" GROUP;   // identifiant unique sur le broker partagé

#define GROUP "g1"   // g1..g4 : doit correspondre à la sonde du même groupe
#define T_TELEMETRY ("market/" GROUP "/fridge/telemetry")
#define T_NOTIFY    ("market/" GROUP "/tab5/notify")
#define T_CMD       ("market/" GROUP "/fridge/cmd")

/* --------------------------- VERROU D'ACCÈS (HMI) -------------------------- */
// [DURCISSEMENT BLEU] : en Phase 1 l'écran est libre. Les bleus ajoutent un PIN/login.
// Laisser REQUIRE_LOGIN=false en Phase 1, passer à true en Phase 2.
#define REQUIRE_LOGIN   false
const char* HMI_PIN   = "2468";              // à changer + stocker hashé idéalement
bool unlocked = !REQUIRE_LOGIN;

/* --------------------------- OBJETS / ÉTAT -------------------------------- */
WiFiClient   net;
PubSubClient mqtt(net);

float  temp = 0, hum = 0, setpoint = 4;
bool   defrost = false, alarmActive = false;
String lastNotify = "";
uint8_t notifyLevel = 0;                      // 0 info / 1 warn / 2 critical
unsigned long lastRx = 0;

/* ------------------------------ AFFICHAGE -------------------------------- */
void drawUI() {
  auto& d = M5.Display;
  d.startWrite();
  d.fillScreen(TFT_BLACK);

  // Bandeau titre
  d.fillRect(0, 0, d.width(), 54, TFT_DARKGREY);
  d.setTextColor(TFT_WHITE); d.setTextSize(3);
  d.setCursor(16, 14); d.print("RAYON FRAIS - Fridge1 (TempRegulator)");

  // Pastille lien MQTT
  bool link = mqtt.connected() && (millis() - lastRx < 6000);
  d.fillCircle(d.width() - 30, 27, 12, link ? TFT_GREEN : TFT_RED);

  // Température (gros chiffre, rouge si alarme)
  d.setTextSize(9);
  d.setTextColor(alarmActive ? TFT_RED : TFT_CYAN);
  d.setCursor(40, 110);
  d.printf("%.1f C", temp);

  // Humidité + consigne
  d.setTextSize(3); d.setTextColor(TFT_WHITE);
  d.setCursor(40, 230); d.printf("Humidite : %.1f %%", hum);
  d.setCursor(40, 275); d.printf("Consigne : %.1f C", setpoint);
  d.setCursor(40, 320);
  d.setTextColor(defrost ? TFT_ORANGE : TFT_GREEN);
  d.printf("Degivrage : %s", defrost ? "ACTIF" : "off");

  // Zone notification
  uint16_t nc = notifyLevel == 2 ? TFT_RED : notifyLevel == 1 ? TFT_ORANGE : TFT_DARKGREEN;
  d.fillRect(0, d.height() - 90, d.width(), 90, nc);
  d.setTextColor(TFT_WHITE); d.setTextSize(3);
  d.setCursor(16, d.height() - 70);
  d.print(lastNotify.length() ? lastNotify : "Systeme nominal");

  // Alarme chaîne du froid
  if (alarmActive) {
    d.setTextSize(4); d.setTextColor(TFT_YELLOW);
    d.setCursor(40, 380); d.print("!! CHAINE DU FROID ROMPUE !!");
  }
  d.endWrite();
}

void drawLock() {
  auto& d = M5.Display;
  d.fillScreen(TFT_NAVY);
  d.setTextColor(TFT_WHITE); d.setTextSize(4);
  d.setCursor(60, 200); d.print("PUPITRE VERROUILLE");
  d.setTextSize(2);
  d.setCursor(60, 270); d.print("Toucher l'ecran pour saisir le code HMI");
}

/* ------------------------------ MQTT RX ---------------------------------- */
void onMessage(char* topic, byte* payload, unsigned int len) {
  lastRx = millis();
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, payload, len)) return;

  if (!strcmp(topic, T_TELEMETRY)) {
    temp     = doc["temp"]     | temp;
    hum      = doc["hum"]      | hum;
    setpoint = doc["setpoint"] | setpoint;
    defrost  = doc["defrost"]  | defrost;
    alarmActive = doc["alarm"] | alarmActive;
  } else if (!strcmp(topic, T_NOTIFY)) {
    // [DURCISSEMENT BLEU] : la notif est affichée telle quelle (confiance aveugle).
    // Un attaquant peut pousser un faux message rassurant pendant qu'il chauffe le rayon.
    lastNotify  = String((const char*) (doc["msg"] | "")) ;
    String lvl  = String((const char*) (doc["level"] | "info"));
    notifyLevel = lvl == "critical" ? 2 : lvl == "warn" ? 1 : 0;
  }
}

void mqttConnect() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  while (!mqtt.connected()) {
    bool ok = (strlen(MQTT_USER) > 0)
                ? mqtt.connect(CLIENT_ID, MQTT_USER, MQTT_PASS)
                : mqtt.connect(CLIENT_ID);
    if (ok) { mqtt.subscribe(T_TELEMETRY); mqtt.subscribe(T_NOTIFY); }
    else    { delay(1500); }
  }
}

void wifiConnect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(300);
}

/* ------------------------------ SAISIE PIN ------------------------------- */
// Clavier minimal : 4 zones tactiles incrémentent 4 chiffres (démo pédagogique).
String typed = "";
bool checkPin() { return typed == HMI_PIN; }

/* ================================ SETUP ================================== */
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.setTextColor(TFT_WHITE);
  wifiConnect();
  mqttConnect();
  if (!unlocked) drawLock();
}

/* ================================= LOOP ================================= */
void loop() {
  M5.update();
  if (WiFi.status() != WL_CONNECTED) wifiConnect();
  if (!mqtt.connected())             mqttConnect();
  mqtt.loop();

  if (!unlocked) {
    // [DURCISSEMENT BLEU] : implémenter ici un vrai pavé numérique + verrouillage
    // après 3 essais. Démo : un appui long déverrouille si PIN par défaut.
    if (M5.Touch.getCount()) {
      typed = HMI_PIN;                 // (placeholder : à remplacer par vraie saisie)
      if (checkPin()) unlocked = true;
    }
    delay(50);
    return;
  }

  static unsigned long last = 0;
  if (millis() - last > 500) { last = millis(); drawUI(); }
}

/* =============================================================================
 *  CHECKLIST DURCISSEMENT (ÉQUIPE BLEUE) sur ce pupitre :
 *   [ ] REQUIRE_LOGIN = true  + vrai pavé PIN + verrouillage après 3 essais   (D9)
 *   [ ] Compte MQTT "hmi" dédié (MQTT_USER/PASS) au lieu de l'anonyme         (D1/D9)
 *   [ ] MQTT_PORT 8883 + WiFiClientSecure + setCACert (vérif. du broker)      (D6)
 *   [ ] Ne plus afficher 'msg' brut : filtrer/limiter longueur + échapper     (D7)
 *   [ ] Ce Tab5 doit être autorisé par filtrage MAC sur le routeur Zyxel      (D8)
 * ============================================================================= */
