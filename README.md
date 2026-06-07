# ESP32 Rocrail Signal Controller 🚦

Un firmware avanzato per microcontrollori ESP32 dedicato alla gestione di segnali ferroviari (scala H0 e altre) tramite il driver PWM I2C **PCA9685**. Il sistema è progettato per integrarsi nativamente con **Rocrail** attraverso il protocollo **MQTT**, garantendo una comunicazione bidirezionale fluida, feedback in tempo reale e un'elevata stabilità del bus dati.

Sviluppato da **Michele Bigi**.

---

## 🌟 Caratteristiche Principali

* **Integrazione Rocrail via MQTT:** Ascolta e processa i comandi standard XML (`<sg>`) per segnali alti di linea (Main) e marmotte di manovra (Shunt).
* **Feedback Bidirezionale & LWT:** Invia conferme di esecuzione a Rocrail. Implementa il *Last Will and Testament* (LWT) per notificare lo stato offline in caso di disconnessione, con logica di fallback automatico non-bloccante.
* **Ottimizzazione I2C (Anti-Spam):** Filtro logico intelligente che previene tempeste di comandi ripetuti sul bus I2C, garantendo una reattività fulminea del microcontrollore.
* **Pannello di Controllo Web:** Un server web integrato e protetto da password per gestire interamente la scheda senza dover mai ricaricare il codice.
* **Mappatura Hardware Dinamica:** Associazione dei pin fisici del PCA9685 ai colori dei segnali (Rosso, Giallo, Verde, ecc.) direttamente dall'interfaccia grafica web.
* **Web Dashboard Interattiva:** Interfaccia per testare la logica dei segnali (aspetti) da smartphone o PC in tempo reale, perfettamente sincronizzata con la centrale Rocrail.
* **Configurazione mDNS & OTA:** Raggiungibile in rete locale tramite `http://nome-scheda.local`. Supporta aggiornamenti firmware Over-The-Air (OTA) protetti da password.
* **Gestione Sicura della Memoria (NVS):** Architettura a compartimenti stagni per il salvataggio su memoria non volatile, che previene corruzioni dei dati di rete e di configurazione hardware.

---

## 🛠️ Requisiti Hardware

* Scheda basata su **ESP32** (es. Wemos D32, NodeMCU ESP32, ecc.)
* Modulo **PCA9685** (Driver PWM I2C a 16 canali)
* Segnali ferroviari a LED ad anodo o catodo comune (richiede cablaggio adeguato al PCA9685)
* Connessione I2C: Collegare i pin `SDA` e `SCL` tra ESP32 e PCA9685.

## 📚 Librerie Software Richieste (Arduino IDE)

Per compilare il codice, assicurati di aver installato tramite il Library Manager di Arduino:
* `PubSubClient` (per la gestione della comunicazione MQTT)
* `Adafruit PWM Servo Driver Library` (per il controllo del modulo PCA9685)

---

## 🚀 Installazione e Primo Avvio

1. **Caricamento del Firmware:** Compila e carica il progetto principale (`.ino`) e i relativi file `.h` sull'ESP32 tramite porta USB.
2. **Modalità Access Point (AP):** Al primo avvio, non trovando reti conosciute, l'ESP32 genererà una propria rete Wi-Fi chiamata `Setup-[nome-scheda]` (di default `Setup-signal`).
3. **Connessione al portale:** * Collegati alla rete Wi-Fi appena creata.
   * Apri il browser e naviga all'indirizzo `http://192.168.4.1` (oppure `http://signal.local`).
4. **Credenziali di Default:**
   * **Utente:** `admin`
   * **Password:** `signal`
5. **Configurazione Iniziale:** * Dalla scheda "Sistema", connetti il modulo alla tua rete Wi-Fi domestica o dedicata al plastico.
   * Modifica il nome del modulo (hostname) e la password di sicurezza per blindare gli accessi e l'OTA.
   * Al riavvio, il modulo si collegherà alla rete specificata e sarà gestibile all'indirizzo `http://[nome-scelto].local`.

---

## ⚙️ Configurazione in Rocrail

Per far comunicare la centrale Rocrail con la scheda:

1. Assicurati che in Rocrail sia correttamente configurato un **Broker MQTT** (es. Mosquitto).
2. Nel pannello Web dell'ESP32 (sezione **MQTT**), inserisci l'indirizzo IP del Broker MQTT o il nome locale (es. `plastico.local`).
3. In Rocrail, configura il tuo segnale e annotane l'ID (es. `sg1`).
4. Nel pannello Web dell'ESP32 (sezione **Hardware**), inserisci l'ID esatto di Rocrail (es. `sg1`), scegli il tipo di segnale (Main o Shunt) e mappa i pin numerici corrispondenti alle uscite sul modulo PCA9685.

Il Wemos si iscrive in automatico al topic `rocrail/service/info/sg` per ricevere i comandi e pubblica i propri feedback di stato su `rocrail/service/client`. Il testamento MQTT (LWT) per il monitoraggio della connessione viene pubblicato su `railway/status/segnali`.

---

## 📡 API JSON Integrata

Il sistema espone un endpoint API per la lettura dello stato in formato JSON, molto utile per integrazioni con dashboard esterne o automazioni (es. Node-RED).

**GET** `http://[nome-scheda].local/api/status`

```json
{
  "uptime": 3600,
  "mqtt_connected": true,
  "segnali": [
    {
      "id": "sg1",
      "tipo": 0,
      "aspetto": 1
    }
  ]
}