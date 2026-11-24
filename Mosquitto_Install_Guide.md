# Guide d'installation & d'utilisation — Mosquitto (broker MQTT)

> **Mosquitto = le broker** (le serveur central qui reçoit et redistribue les messages).
> **paho-mqtt = une librairie cliente** (pour écrire ses propres programmes). Les deux vont
> ensemble : le broker héberge, les clients (ESP, Tab5, Node-RED, scripts) se connectent.

---

## 1. Installation

### Windows
1. Télécharger l'installeur **Eclipse Mosquitto** : <https://mosquitto.org/download>
2. Les binaires sont dans `C:\Program Files\mosquitto` (ajoutez-le au PATH, ou placez-vous
   dans ce dossier).
3. Vérifier :
   ```bat
   mosquitto -h
   mosquitto_pub --help
   ```

### Linux — Debian / Ubuntu / Kali
```bash
sudo apt update
sudo apt install -y mosquitto mosquitto-clients
```
`mosquitto` installe le **broker** ; `mosquitto-clients` installe les outils en ligne de
commande `mosquitto_pub` et `mosquitto_sub`.

### Linux — Fedora / RHEL / CentOS
```bash
sudo dnf install -y mosquitto
```

### macOS (Homebrew)
```bash
brew install mosquitto
```

---

## 2. Démarrer le broker

### Linux — en service (systemd)
```bash
sudo systemctl start mosquitto      # démarrer
sudo systemctl enable mosquitto     # démarrage automatique au boot
sudo systemctl status mosquitto     # vérifier l'état
sudo systemctl restart mosquitto    # recharger après modification de la config
sudo journalctl -u mosquitto -f     # voir les logs en direct
```

### Linux / Windows — en console (verbeux, pratique pour le TP)
```bash
mosquitto -v                                              # config par défaut, verbeux
mosquitto -v -c /etc/mosquitto/mosquitto.conf             # Linux, config choisie
```
```bat
mosquitto -v -c "C:\Program Files\mosquitto\mosquitto.conf"   REM Windows
```

### Windows — en service
```bat
net start mosquitto
net stop mosquitto
```

### Config minimale « Phase 1 » (labo, anonyme)
Dans le fichier `.conf` :
```conf
listener 1883
allow_anonymous true
```
> ⚠️ `allow_anonymous true` est **volontairement non sécurisé** pour le TP. Voir la Phase 2
> ci-dessous pour le durcissement.

---

## 3. Tester : s'abonner / publier

**S'abonner** (laisser cette fenêtre ouverte pour observer) :
```bash
mosquitto_sub -h 127.0.0.1 -t "market/g1/fridge/telemetry" -v
mosquitto_sub -h 127.0.0.1 -t "#" -v        # écouter TOUT (joker)
```

**Publier** (dans une autre fenêtre) :
```bash
mosquitto_pub -h 127.0.0.1 -t "market/g1/fridge/telemetry" -m "{\"temp\":4,\"pressure\":1013}"
mosquitto_pub -h 127.0.0.1 -t "market/g1/fridge/cmd" -m "{\"defrost\":true}"
```

---

## 4. Durcissement — « Phase 2 » (fichiers fournis dans `nodered/blue_team/`)

| Objectif | Réglage |
|---|---|
| Fin de l'anonyme | `allow_anonymous false` + `password_file` (créé avec `mosquitto_passwd`) |
| Droits par rôle | `acl_file` (lire/écrire par topic) |
| Chiffrement | `listener 8883` + `cafile` / `certfile` / `keyfile` (TLS) |
| Anti-DoS | `max_connections`, `max_queued_messages`, `message_size_limit` |

Créer des comptes :
```bash
sudo mosquitto_passwd -c /etc/mosquitto/passwd sensor      # 1er compte (-c crée le fichier)
sudo mosquitto_passwd    /etc/mosquitto/passwd regulator   # comptes suivants (sans -c)
```
Puis **redémarrer** le broker. Fichiers prêts à l'emploi :
`mosquitto_hardened.conf` (Linux), `mosquitto_hardened_windows.conf` (Windows),
`aclfile`, `aclfile_multigroupe`.

Tester en TLS :
```bash
mosquitto_sub -h 127.0.0.1 -p 8883 --cafile ca.crt -u sensor -P <mdp> -t "market/g1/#" -v
```

---

## 5. Mosquitto vs paho-mqtt (rappel)

| | Rôle | Exemple |
|---|---|---|
| **Mosquitto** | **Broker** (serveur) + outils clients CLI | `mosquitto`, `mosquitto_pub`, `mosquitto_sub` |
| **paho-mqtt** | **Librairie cliente** (Python) | `import paho.mqtt.client` dans un script |

Un client seul ne sert à rien sans broker. Pour le TP : on lance **d'abord** Mosquitto, on
ouvre un `mosquitto_sub` pour observer, puis on publie (CLI ou script Python).
