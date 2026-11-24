# Outils d'attaque — Kali Linux est-il nécessaire ?

> ⚠️ **Usage strictement pédagogique, en laboratoire isolé.** Voir `../DISCLAIMER.md`.

## Réponse courte : NON pour l'essentiel.

La majorité des attaques du TP passent par **MQTT** et s'exécutent avec un simple script
**Python** multi-plateforme. **Kali Linux n'est requis que pour les attaques Wi-Fi/radio.**

| Attaque | Outil | Kali / Linux requis ? |
|---|---|---|
| Reconnaissance (topics, ports) | `mqtt_pentest.py` / `mosquitto_sub` | **Non** — Python / CLI, multi-OS |
| Bruteforce & accès anonyme MQTT | `mqtt_pentest.py` | **Non** |
| Spoofing capteur / injection | `mqtt_pentest.py` | **Non** |
| Déni de service (flood MQTT) | `mqtt_pentest.py` | **Non** |
| Exfiltration (`#`) | `mqtt_pentest.py` | **Non** |
| Faux broker | `mosquitto` | **Non** (Mosquitto multi-OS) |
| **Accès Wi-Fi (capture/crack WPA2)** | `recon.sh` (airodump/aircrack) | **Oui** — Linux + carte Wi-Fi compatible mode moniteur |
| **MITM (ARP spoofing)** | bettercap + Wireshark | **Oui** — Linux recommandé |

Autrement dit :
- **`mqtt_pentest.py`** : 100 % Python, tourne sous **Windows, Linux et macOS**. Aucun Kali.
- **`recon.sh`** : script Bash. La partie *réseau* (`nmap`, `mosquitto_sub`) marche partout ;
  la partie *Wi-Fi* (`airmon-ng`, `airodump-ng`, `aircrack-ng`) nécessite **Linux** (Kali
  tout prêt, ou toute distribution avec `aircrack-ng`) et une **carte Wi-Fi compatible**.

## Prérequis & utilisation

```bash
pip install paho-mqtt          # seule dépendance de mqtt_pentest.py
python3 mqtt_pentest.py        # régler TARGET (IP du broker) et GROUP (g1..g4) en tête du fichier
```
## Liens pref. Arduino
https://arduino.esp8266.com/stable/package_esp8266com_index.json
https://dl.espressif.com/dl/package_esp32_index.json
https://espressif.github.io/arduino-esp32/package_esp32_index.json
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
Sous Windows : `python mqtt_pentest.py` (ou `py mqtt_pentest.py`).

> Pour un montage **tout en Wi-Fi** (partage de connexion téléphone), les attaques Wi-Fi/MITM
> ci-dessus peuvent être laissées en **démonstration théorique** : voir
> `../Attaques_Parades_Questions_Corrige.pdf`.

