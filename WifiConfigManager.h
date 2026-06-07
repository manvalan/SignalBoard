#ifndef WIFICONFIGMANAGER_H
#define WIFICONFIGMANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <functional>
#include "WebPages.h"

class WifiConfigManager {
  private:
    WebServer server;
    Preferences preferences;
    bool setupMode;
    IPAddress brokerIP;
    
    String ssid;
    String password;
    String mqtt_server_name;
    String hostname;
    String webUser = "admin";
    String webPass;

    std::function<void(int, bool)> onTestPin;
    std::function<String()> onGetStatusJSON;
    std::function<void(String, int)> onTestSignalLogic;

    bool checkAuth() {
        if (!server.authenticate(webUser.c_str(), webPass.c_str())) {
            server.requestAuthentication();
            return false;
        }
        return true;
    }

    void startAP() {
        setupMode = true;
        WiFi.disconnect(true);
        delay(100);
        WiFi.mode(WIFI_AP_STA);
        String apName = "Setup-" + hostname;
        WiFi.softAP(apName.c_str());
        Serial.printf("\n[AP Mode] Rete creata: %s\n", apName.c_str());
        Serial.print("IP di configurazione: ");
        Serial.println(WiFi.softAPIP());

        if (MDNS.begin(hostname.c_str())) {
            MDNS.addService("http", "tcp", 80);
            Serial.printf("[mDNS] Attivo in Setup! Digita: http://%s.local\n", hostname.c_str());
        }
    }

    void connectSTA() {
        Serial.printf("[Wi-Fi] Tento la connessione a: %s\n", ssid.c_str());
        WiFi.setHostname(hostname.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());

        int tentativi = 0;
        while (WiFi.status() != WL_CONNECTED && tentativi < 20) { delay(500); Serial.print("."); tentativi++; }

        if (WiFi.status() == WL_CONNECTED) {
            setupMode = false;
            Serial.println("\n[Wi-Fi] Connesso!");
            
            ArduinoOTA.setHostname(hostname.c_str());
            ArduinoOTA.setPassword(webPass.c_str());
            ArduinoOTA.begin();
            Serial.println("[OTA] Servizio avviato e protetto da password.");

            if (MDNS.begin(hostname.c_str())) {
                MDNS.addService("http", "tcp", 80);
                Serial.printf("[mDNS] Attivo in Rete! Digita: http://%s.local\n", hostname.c_str());
                
                IPAddress ip;
                if (ip.fromString(mqtt_server_name)) {
                    brokerIP = ip;
                } else {
                    brokerIP = MDNS.queryHost(mqtt_server_name);
                    int mDnsAttempts = 0;
                    while (brokerIP.toString() == "0.0.0.0" && mDnsAttempts < 5) {
                        delay(2000); brokerIP = MDNS.queryHost(mqtt_server_name); mDnsAttempts++;
                    }
                }
            }
        } else {
            startAP();
        }
    }

    void setupRoutes() {
        server.on("/", HTTP_GET, [this]() {
            if (!checkAuth()) return; 
            String html = String(html_header) + String(system_html);
            String currentNetwork = (WiFi.status() == WL_CONNECTED) ? "Connesso a <b>" + WiFi.SSID() + "</b>" : "Non connesso (Modalità AP)";
            html.replace("%WIFI_STATUS%", currentNetwork);
            html.replace("%HOSTNAME%", hostname);
            server.send(200, "text/html", html);
        });

        server.on("/dashboard", HTTP_GET, [this]() {
            if (!checkAuth()) return;
            server.send(200, "text/html", String(html_header) + String(dashboard_html));
        });

        server.on("/api/status", HTTP_GET, [this]() {
            if (!checkAuth()) return;
            if (onGetStatusJSON) {
                server.send(200, "application/json", onGetStatusJSON());
            } else {
                server.send(500, "application/json", "{\"error\":\"Callback JSON non definita\"}");
            }
        });

        server.on("/test_signal", HTTP_GET, [this]() {
            if (!checkAuth()) return;
            if (server.hasArg("id") && server.hasArg("aspect")) {
                if (onTestSignalLogic) {
                    onTestSignalLogic(server.arg("id"), server.arg("aspect").toInt());
                }
                server.send(200, "text/plain", "Comando Logico Inviato");
            } else { 
                server.send(400, "text/plain", "Parametri Mancanti"); 
            }
        });

        server.on("/save_security", HTTP_POST, [this]() {
            if (!checkAuth()) return;
            preferences.begin("network", false);
            if (server.hasArg("hostname") && server.arg("hostname") != "") preferences.putString("hostname", server.arg("hostname"));
            if (server.hasArg("web_pass") && server.arg("web_pass") != "") preferences.putString("web_pass", server.arg("web_pass"));
            preferences.end();
            server.send(200, "text/html", "<meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'><h2>💾 Sicurezza Aggiornata!</h2><p>Riavvio in corso...</p>");
            delay(1000); ESP.restart();
        });

        server.on("/scan_wifi", HTTP_GET, [this]() {
            if (!checkAuth()) return;
            int n = WiFi.scanNetworks();
            String options = "<option value=''>-- Seleziona una rete --</option>";
            for (int i = 0; i < n; ++i) options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + "dBm)</option>";
            server.send(200, "text/plain", options);
        });

        server.on("/save_wifi", HTTP_POST, [this]() {
            if (!checkAuth()) return;
            String newSsid = server.arg("ssid_manual") != "" ? server.arg("ssid_manual") : server.arg("ssid");
            String newPass = server.arg("password");
            if (newSsid != "") {
                preferences.begin("network", false); 
                preferences.putString("ssid", newSsid);
                preferences.putString("password", newPass);
                preferences.end();
                server.send(200, "text/html", "<meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'><h2>💾 Wi-Fi Salvato!</h2><p>Riavvio...</p>");
                delay(1000); ESP.restart();
            }
        });

        server.on("/hard_reset", HTTP_GET, [this]() {
            if (!checkAuth()) return;
            preferences.begin("network", false); preferences.clear(); preferences.end();
            preferences.begin("railway", false); preferences.clear(); preferences.end();
            server.send(200, "text/html", "<meta charset='UTF-8'><meta http-equiv='refresh' content='4;url=/'><h2 style='color:red;'>HARD RESET COMPLETATO</h2>");
            delay(1500); ESP.restart();
        });

        server.on("/mqtt", HTTP_GET, [this]() {
            if (!checkAuth()) return;
            preferences.begin("network", true); 
            String currentMqtt = preferences.getString("mqtt_host", "plastico.local"); 
            preferences.end();
            server.send(200, "text/html", String(html_header) + String(mqtt_html_top) + currentMqtt + String(mqtt_html_bottom));
        });

        server.on("/save_mqtt", HTTP_POST, [this]() {
            if (!checkAuth()) return;
            if (server.hasArg("mqtt_server")) {
                preferences.begin("network", false); 
                preferences.putString("mqtt_host", server.arg("mqtt_server")); 
                preferences.end();
                server.send(200, "text/html", "<meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'><h2>💾 MQTT Salvato!</h2>");
                delay(1000); ESP.restart();
            }
        });

        server.on("/info", HTTP_GET, [this]() {
            if (!checkAuth()) return;
            server.send(200, "text/html", String(html_header) + String(info_html));
        });

        server.on("/test_pin", HTTP_GET, [this]() {
            if (!checkAuth()) return;
            if (server.hasArg("pin") && server.hasArg("state")) {
                if (onTestPin) onTestPin(server.arg("pin").toInt(), server.arg("state").toInt() == 1);
                server.send(200, "text/plain", "OK");
            }
        });

        server.on("/mapping", HTTP_GET, [this]() {
            if (!checkAuth()) return;
            String html = String(html_header) + String(mapping_script_and_legend);
            preferences.begin("railway", true);
            for (int i = 1; i <= 4; i++) {
                String idKey = "id_" + String(i), tipoKey = "tipo_" + String(i);
                String pinRKey = "pinR_" + String(i), pinGKey = "pinG_" + String(i), pinVKey = "pinV_" + String(i);
                String idVal = preferences.getString(idKey.c_str(), "");
                int tipoVal = preferences.getInt(tipoKey.c_str(), -1);
                int pinRVal = preferences.getInt(pinRKey.c_str(), 0);
                int pinGVal = preferences.getInt(pinGKey.c_str(), 0);
                int pinVVal = preferences.getInt(pinVKey.c_str(), 0);

                html += "<div class='signal-card'><h3>Slot " + String(i) + "</h3>";
                html += "<label>ID Rocrail:</label><input type='text' name='" + idKey + "' value='" + idVal + "'>";
                html += "<label>Tipo Segnale:</label><select name='" + tipoKey + "'>";
                html += "<option value='-1'" + String(tipoVal == -1 ? " selected" : "") + ">Nessuno (Disabilitato)</option>";
                html += "<option value='0'" + String(tipoVal == 0 ? " selected" : "") + ">Segnale Alto (Main)</option>";
                html += "<option value='1'" + String(tipoVal == 1 ? " selected" : "") + ">Marmotta (Shunt)</option></select>";

                html += "<div class='pin-group'>";
                html += "<div>🔴 Rosso / C<input type='number' name='" + pinRKey + "' value='" + String(pinRVal) + "'>";
                html += "<button type='button' class='test-btn' onclick='testPin(\"" + pinRKey + "\", this, \"#F44336\")'>TEST</button></div>";
                html += "<div>🟡 Giallo / B<input type='number' name='" + pinGKey + "' value='" + String(pinGVal) + "'>";
                html += "<button type='button' class='test-btn' onclick='testPin(\"" + pinGKey + "\", this, \"#FFC107\")'>TEST</button></div>";
                html += "<div>🟢 Verde / A<input type='number' name='" + pinVKey + "' value='" + String(pinVVal) + "'>";
                html += "<button type='button' class='test-btn' onclick='testPin(\"" + pinVKey + "\", this, \"#4CAF50\")'>TEST</button></div>";
                html += "</div></div>";
            }
            preferences.end();
            html += "<input type='submit' value='💾 SALVA MAPPATURA'></form></body></html>";
            server.send(200, "text/html", html);
        });

        server.on("/save_mapping", HTTP_POST, [this]() {
            if (!checkAuth()) return;
            preferences.begin("railway", false);
            for (int i = 1; i <= 4; i++) {
                if (server.hasArg("id_" + String(i))) preferences.putString(("id_" + String(i)).c_str(), server.arg("id_" + String(i)));
                if (server.hasArg("tipo_" + String(i))) preferences.putInt(("tipo_" + String(i)).c_str(), server.arg("tipo_" + String(i)).toInt());
                if (server.hasArg("pinR_" + String(i))) preferences.putInt(("pinR_" + String(i)).c_str(), server.arg("pinR_" + String(i)).toInt());
                if (server.hasArg("pinG_" + String(i))) preferences.putInt(("pinG_" + String(i)).c_str(), server.arg("pinG_" + String(i)).toInt());
                if (server.hasArg("pinV_" + String(i))) preferences.putInt(("pinV_" + String(i)).c_str(), server.arg("pinV_" + String(i)).toInt());
            }
            preferences.end();
            server.send(200, "text/html", "<meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'><h2>💾 Mappatura Salvata!</h2>");
            delay(1000); ESP.restart();
        });
    }

  public:
    WifiConfigManager() : server(80), setupMode(false) {}

    void setTestCallback(std::function<void(int, bool)> cb) { onTestPin = cb; }
    void setJsonCallback(std::function<String()> cb) { onGetStatusJSON = cb; }
    void setTestSignalCallback(std::function<void(String, int)> cb) { onTestSignalLogic = cb; }

    void begin() {
        preferences.begin("network", true); 
        ssid = preferences.getString("ssid", "");
        password = preferences.getString("password", "");
        mqtt_server_name = preferences.getString("mqtt_host", "plastico.local"); 
        hostname = preferences.getString("hostname", "signal");
        webPass = preferences.getString("web_pass", "signal");
        preferences.end();

        if (ssid == "") startAP(); else connectSTA();
        
        setupRoutes();
        server.begin();
    }

    void loop() { 
        server.handleClient(); 
        if (!setupMode && WiFi.status() == WL_CONNECTED) {
            ArduinoOTA.handle();
        }
    }
    
    bool isSetupMode() const { return setupMode; }
    IPAddress getBrokerIP() const { return brokerIP; }
};

#endif