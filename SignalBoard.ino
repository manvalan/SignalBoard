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

// Variabili per l'ottimizzazione
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
    int inizio = xml.indexOf(attributo + "=\"");
    if (inizio == -1) return "";
    inizio += attributo.length() + 2;
    int fine = xml.indexOf("\"", inizio);
    return xml.substring(inizio, fine);
}

void inviaFeedbackRocrail(String id, int aspetto, int tipo) {
    if (!mqttClient.connected()) return;
    String stateStr = "red"; 
    
    if (tipo == 0) {
        if (aspetto == 1) stateStr = "green";
        else if (aspetto == 2) stateStr = "yellow";
    } else {
        if (aspetto == 4) stateStr = "green";
        else if (aspetto == 5) stateStr = "yellow";
    }
    String xml = "<sg id=\"" + id + "\" state=\"" + stateStr + "\"/>";
    mqttClient.publish(topic_client, xml.c_str());
}

// OTTIMIZZATO: Niente più tempeste I2C e stringhe lente
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Parsing molto più veloce e leggero per la RAM
    char msgBuffer[length + 1];
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

                // FILTRO ANTI-SPAM: Ignora i messaggi doppi di Rocrail
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
    String json = "{";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"mqtt_connected\":" + String(mqttClient.connected() ? "true" : "false") + ",";
    json += "\"segnali\":[";
    
    for (size_t i = 0; i < segnaliAttivi.size(); i++) {
        json += "{\"id\":\"" + segnaliAttivi[i].id + "\",";
        json += "\"tipo\":" + String(segnaliAttivi[i].tipo) + ",";
        json += "\"aspetto\":" + String(segnaliAttivi[i].aspettoAttuale) + "}";
        if (i < segnaliAttivi.size() - 1) json += ","; 
    }
    json += "]}";
    return json;
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

void webTestPin(int pin, bool state) {
    pca.setPWM(pin, 0, state ? 4095 : 0);
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
        // Cacheiamo le stringhe lente all'avvio
        cachedHostname = networkConfig.readString("hostname", "signal");
        String broker = networkConfig.readString("mqtt_host", "plastico.local");
        
        mqttClient.setServer(broker.c_str(), mqtt_port);
        mqttClient.setCallback(mqttCallback);

        for (int i = 1; i <= 4; i++) {
            String baseKey = "id_" + String(i);
            String id = hardwareConfig.readString(baseKey.c_str(), "");
            
            if (id != "") {
                int tipo = hardwareConfig.readInt(("tipo_" + String(i)).c_str(), 0);
                int pinR = hardwareConfig.readInt(("pinR_" + String(i)).c_str(), 0);
                int pinG = hardwareConfig.readInt(("pinG_" + String(i)).c_str(), 0);
                int pinV = hardwareConfig.readInt(("pinV_" + String(i)).c_str(), 0);

                SignalFS* hw = new SignalFS((SignalType)tipo, &pca, pinR, pinG, pinV);
                hw->begin();
                segnaliAttivi.push_back({id, tipo, hw, -1}); 
            }
        }
    }
}

// OTTIMIZZATO: Nessun blocco nel loop
void loop() {
    netManager.loop();
    
    if (!netManager.isSetupMode()) {
        if (!mqttClient.connected()) {
            unsigned long now = millis();
            // Riprova ogni 5 secondi, non intasa il processore
            if (now - lastMqttReconnectAttempt > 5000) {
                lastMqttReconnectAttempt = now;
                String lwtMsg = "{\"module\":\"" + cachedHostname + "\", \"status\":\"offline\"}";
                
                if (mqttClient.connect(cachedHostname.c_str(), NULL, NULL, topic_lwt, 0, true, lwtMsg.c_str())) {
                    mqttClient.subscribe(topic_sub);
                    String onlineMsg = "{\"module\":\"" + cachedHostname + "\", \"status\":\"online\"}";
                    mqttClient.publish(topic_lwt, onlineMsg.c_str(), true); 
                    Serial.println("[MQTT] Riconnesso a Rocrail.");
                } else {
                    if (mqttClient.connect(cachedHostname.c_str())) {
                        mqttClient.subscribe(topic_sub);
                        Serial.println("[MQTT] Riconnesso a Rocrail (No LWT).");
                    }
                }
            }
        } else {
            // Eseguiamo il loop MQTT solo se effettivamente connessi
            mqttClient.loop();
        }
    }
}