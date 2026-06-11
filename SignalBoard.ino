#include <WiFi.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include <Adafruit_PWMServoDriver.h>
#include "NVSManager.h"
#include "WifiConfigManager.h"
#include "SignalFS.h"
#include <vector>

NVSManager networkConfig("network");
NVSManager hardwareConfig("railway");

WifiConfigManager netManager;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);

const int mqtt_port = 1883;
const char* topic_sub = "rocrail/service/info/sg";
const char* topic_client = "rocrail/service/client";
const char* topic_lwt = "railway/status/segnali";

unsigned long lastMqttReconnectAttempt = 0;
String cachedHostname = "signal";

struct SegnaleConfigurato {
  String id;
  int tipo;
  SignalFS* hardware;
  int aspettoAttuale;
};
std::vector<SegnaleConfigurato> segnaliAttivi;

String estraiAttributo(String xml, String attributo) {
  String search = " " + attributo + "=\"";
  int inizio = xml.indexOf(search);
  if (inizio == -1) return "";
  inizio += search.length();
  int fine = xml.indexOf("\"", inizio);
  if (fine == -1) return "";
  return xml.substring(inizio, fine);
}

void inviaFeedbackRocrail(String id, int aspetto, int tipo) {
  if (!mqttClient.connected()) return;
  String cmdStr = "red";

  if (tipo == 0) {
    if (aspetto == 1) cmdStr = "green";
    else if (aspetto == 2) cmdStr = "yellow";
  } else {
    if (aspetto == 4) cmdStr = "green";
    else if (aspetto == 5) cmdStr = "yellow";
  }

  String xml = "<sg id=\"" + id + "\" cmd=\"" + cmdStr + "\"/>";
  mqttClient.publish(topic_client, xml.c_str());
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (length > 512) return;
  char msgBuffer[513];
  memcpy(msgBuffer, payload, length);
  msgBuffer[length] = '\0';
  String msg = String(msgBuffer);

  if (msg.indexOf("<sg ") != -1) {
    String id = estraiAttributo(msg, "id");
    String stato = estraiAttributo(msg, "state");

    for (auto& s : segnaliAttivi) {
      if (s.id == id) {
        int aspNum = -1;
        if (s.tipo == 0) {
          if (stato == "red") aspNum = 0;
          else if (stato == "green") aspNum = 1;
          else if (stato == "yellow") aspNum = 2;
        } else if (s.tipo == 1) {
          if (stato == "red") aspNum = 3;
          else if (stato == "green") aspNum = 4;
          else if (stato == "yellow") aspNum = 5;
        }

        if (aspNum != -1 && s.aspettoAttuale != aspNum) {
          s.hardware->setAspect((SignalAspect)aspNum);
          s.aspettoAttuale = aspNum;
          Serial.printf(">>> Rocrail -> Segnale %s cambiato su %d\n", id.c_str(), aspNum);
        }
        break;
      }
    }
  }
}

String generaStatoJSON() {
  char buf[384];
  int pos = snprintf(buf, sizeof(buf),
                     "{\"uptime\":%lu,\"mqtt_connected\":%s,\"ip\":\"%s\",\"rssi\":%d,\"segnali\":[",
                     millis() / 1000,
                     mqttClient.connected() ? "true" : "false",
                     WiFi.localIP().toString().c_str(),
                     WiFi.RSSI());

  for (size_t i = 0; i < segnaliAttivi.size(); i++) {
    if (i > 0) pos += snprintf(buf + pos, sizeof(buf) - pos, ",");
    pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "{\"id\":\"%s\",\"tipo\":%d,\"aspetto\":%d}",
                    segnaliAttivi[i].id.c_str(),
                    segnaliAttivi[i].tipo,
                    segnaliAttivi[i].aspettoAttuale);
  }
  snprintf(buf + pos, sizeof(buf) - pos, "]}");
  return String(buf);
}

void eseguiTestLogicoWeb(String id, int aspetto) {
  for (auto& s : segnaliAttivi) {
    if (s.id == id) {
      if (s.aspettoAttuale != aspetto) {
        s.hardware->setAspect((SignalAspect)aspetto);
        s.aspettoAttuale = aspetto;
        inviaFeedbackRocrail(id, aspetto, s.tipo);
      }
      break;
    }
  }
}

// FUNZIONE AGGIORNATA: Riceve la luminosità dalla pagina Web!
void webTestPin(int pin, bool state, int brightness) {
  pca.setPWM(pin, 0, state ? brightness : 0);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pca.begin();
  pca.setPWMFreq(1000);

  netManager.setTestCallback(webTestPin);
  netManager.setTestSignalCallback(eseguiTestLogicoWeb);
  netManager.setJsonCallback(generaStatoJSON);

  netManager.begin();

  if (!netManager.isSetupMode()) {
    cachedHostname = networkConfig.readString("hostname", "signal");
    mqttClient.setBufferSize(512);
    mqttClient.setCallback(mqttCallback);

    for (int i = 1; i <= 5; i++) {
      String baseKey = "id_" + String(i);
      String id = hardwareConfig.readString(baseKey.c_str(), "");

      if (id != "") {
        int tipo = hardwareConfig.readInt(("tipo_" + String(i)).c_str(), 0);
        int pinR = hardwareConfig.readInt(("pinR_" + String(i)).c_str(), 0);
        int pinG = hardwareConfig.readInt(("pinG_" + String(i)).c_str(), 0);
        int pinV = hardwareConfig.readInt(("pinV_" + String(i)).c_str(), 0);
        int brR = hardwareConfig.readInt(("brR_" + String(i)).c_str(), 4095);
        int brG = hardwareConfig.readInt(("brG_" + String(i)).c_str(), 4095);
        int brV = hardwareConfig.readInt(("brV_" + String(i)).c_str(), 4095);

        SignalFS* hw = new SignalFS((SignalType)tipo, &pca, pinR, pinG, pinV, brR, brG, brV);
        hw->begin();

        int aspettoIniziale = (tipo == 0) ? ASPECT_RED : ASPECT_STOP;
        hw->setAspect((SignalAspect)aspettoIniziale);
        segnaliAttivi.push_back({ id, tipo, hw, aspettoIniziale });
      }
    }
  }
}

void loop() {
  netManager.loop();

  if (!netManager.isSetupMode()) {
    if (!mqttClient.connected()) {
      unsigned long now = millis();

      // Attende 15 secondi tra i tentativi in modo che il web resti velocissimo
      if (now - lastMqttReconnectAttempt > 15000) {
        lastMqttReconnectAttempt = now;

        // --- TRADUZIONE mDNS AL VOLO ---
        IPAddress brokerIP = netManager.getBrokerIP();

        if (brokerIP != IPAddress(0, 0, 0, 0)) {
          mqttClient.setServer(brokerIP, mqtt_port);

          String lwtMsg = "{\"module\":\"" + cachedHostname + "\", \"status\":\"offline\"}";
          if (mqttClient.connect(cachedHostname.c_str(), NULL, NULL, topic_lwt, 0, true, lwtMsg.c_str())) {
            mqttClient.subscribe(topic_sub);
            String onlineMsg = "{\"module\":\"" + cachedHostname + "\", \"status\":\"online\"}";
            mqttClient.publish(topic_lwt, onlineMsg.c_str(), true);
            Serial.println("[MQTT] Connesso a Rocrail.");
          } else {
            Serial.printf("[MQTT] Connessione fallita (rc=%d). Riprovo tra 15s...\n", mqttClient.state());
          }
        } else {
          Serial.println("[MQTT] mDNS non ha trovato Rocrail. Riprovo tra 15s...");
        }
      }
    } else {
      mqttClient.loop();
    }
  }
}