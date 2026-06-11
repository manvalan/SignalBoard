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
    
    String ssid;
    String password;
    String mqtt_server_name;
    String hostname;
    String webUser = "admin";
    String webPass;

    // Callback aggiornata per accettare la luminosità
    std::function<void(int, bool, int)> onTestPin;
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
        if (MDNS.begin(hostname.c_str())) { MDNS.addService("http", "tcp", 80); }
    }

    void connectSTA() {
        WiFi.setHostname(hostname.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());

        int tentativi = 0;
        while (WiFi.status() != WL_CONNECTED && tentativi < 20) { delay(500); tentativi++; }

        if (WiFi.status() == WL_CONNECTED) {
            setupMode = false;
            ArduinoOTA.setHostname(hostname.c_str());
            ArduinoOTA.setPassword(webPass.c_str());
            ArduinoOTA.begin();
            if (MDNS.begin(hostname.c_str())) { MDNS.addService("http", "tcp", 80); }
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
            if (onGetStatusJSON) { server.send(200, "application/json", onGetStatusJSON()); }
        });

        server.on("/test_signal", HTTP_GET, [this]() {
            if (!checkAuth()) return;
            if (server.hasArg("id") && server.hasArg("aspect")) {
                int aspect = server.arg("aspect").toInt();
                if (aspect < 0 || aspect > 5) { server.send(400, "text/plain", "Aspetto non valido"); return; }
                if (onTestSignalLogic) onTestSignalLogic(server.arg("id"), aspect);
                server.send(200, "text/plain", "Comando Logico Inviato");
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

        server.on("/hard_reset", HTTP_POST, [this]() {
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
                int pin = server.arg("pin").toInt();
                if (pin < 0 || pin > 15) { server.send(400, "text/plain", "Pin non valido"); return; }
                int br = server.hasArg("br") ? server.arg("br").toInt() : 4095;
                if (br < 0) br = 0; if (br > 4095) br = 4095;
                if (onTestPin) onTestPin(pin, server.arg("state").toInt() == 1, br);
                server.send(200, "text/plain", "OK");
            }
        });

        server.on("/mapping", HTTP_GET, [this]() {
            if (!checkAuth()) return;
            String html = String(html_header) + String(mapping_script_and_legend);
            preferences.begin("railway", true);
            for (int i = 1; i <= 5; i++) {
                String idKey = "id_" + String(i), tipoKey = "tipo_" + String(i);
                String pinRKey = "pinR_" + String(i), pinGKey = "pinG_" + String(i), pinVKey = "pinV_" + String(i);
                String brRKey = "brR_" + String(i), brGKey = "brG_" + String(i), brVKey = "brV_" + String(i); // Chiavi Luminosità
                
                String idVal = preferences.getString(idKey.c_str(), "");
                int tipoVal = preferences.getInt(tipoKey.c_str(), -1);
                int pinRVal = preferences.getInt(pinRKey.c_str(), 0);
                int pinGVal = preferences.getInt(pinGKey.c_str(), 0);
                int pinVVal = preferences.getInt(pinVKey.c_str(), 0);
                // Legge i valori salvati, altrimenti usa 4095 (max)
                int brRVal = preferences.getInt(brRKey.c_str(), 4095);
                int brGVal = preferences.getInt(brGKey.c_str(), 4095);
                int brVVal = preferences.getInt(brVKey.c_str(), 4095);

                html += "<div class='signal-card'><h3>Slot " + String(i) + "</h3>";
                html += "<label>ID Rocrail:</label><input type='text' name='" + idKey + "' value='" + idVal + "'>";
                html += "<label>Tipo Segnale:</label><select name='" + tipoKey + "'>";
                html += "<option value='-1'" + String(tipoVal == -1 ? " selected" : "") + ">Nessuno (Disabilitato)</option>";
                html += "<option value='0'" + String(tipoVal == 0 ? " selected" : "") + ">Segnale Alto (Main)</option>";
                html += "<option value='1'" + String(tipoVal == 1 ? " selected" : "") + ">Marmotta (Shunt)</option></select>";

                html += "<div class='pin-group'>";
                html += "<div>🔴 Rosso / C<input type='number' name='" + pinRKey + "' value='" + String(pinRVal) + "'>";
                html += "<input type='range' name='" + brRKey + "' min='0' max='4095' value='" + String(brRVal) + "' title='Luminosità'>";
                html += "<button type='button' class='test-btn' onclick='testPin(\"" + pinRKey + "\", \"" + brRKey + "\", this, \"#F44336\")'>TEST</button></div>";
                
                html += "<div>🟡 Giallo / B<input type='number' name='" + pinGKey + "' value='" + String(pinGVal) + "'>";
                html += "<input type='range' name='" + brGKey + "' min='0' max='4095' value='" + String(brGVal) + "' title='Luminosità'>";
                html += "<button type='button' class='test-btn' onclick='testPin(\"" + pinGKey + "\", \"" + brGKey + "\", this, \"#FFC107\")'>TEST</button></div>";
                
                html += "<div>🟢 Verde / A<input type='number' name='" + pinVKey + "' value='" + String(pinVVal) + "'>";
                html += "<input type='range' name='" + brVKey + "' min='0' max='4095' value='" + String(brVVal) + "' title='Luminosità'>";
                html += "<button type='button' class='test-btn' onclick='testPin(\"" + pinVKey + "\", \"" + brVKey + "\", this, \"#4CAF50\")'>TEST</button></div>";
                html += "</div></div>";
            }
            preferences.end();
            html += "<input type='submit' value='💾 SALVA MAPPATURA E LUMINOSITÀ'></form></body></html>";
            server.send(200, "text/html", html);
        });

        
        server.on("/save_mapping", HTTP_POST, [this]() {
            if (!checkAuth()) return;
            preferences.begin("railway", false);
            for (int i = 1; i <= 5; i++) {
                // Salva ID e Tipo
                if (server.hasArg("id_" + String(i))) preferences.putString(("id_" + String(i)).c_str(), server.arg("id_" + String(i)));
                if (server.hasArg("tipo_" + String(i))) preferences.putInt(("tipo_" + String(i)).c_str(), server.arg("tipo_" + String(i)).toInt());
                
                // Salva i canali fisici
                if (server.hasArg("pinR_" + String(i))) preferences.putInt(("pinR_" + String(i)).c_str(), server.arg("pinR_" + String(i)).toInt());
                if (server.hasArg("pinG_" + String(i))) preferences.putInt(("pinG_" + String(i)).c_str(), server.arg("pinG_" + String(i)).toInt());
                if (server.hasArg("pinV_" + String(i))) preferences.putInt(("pinV_" + String(i)).c_str(), server.arg("pinV_" + String(i)).toInt());
                
                // ---> IL PEZZO CHE MANCAVA: SALVA LA LUMINOSITA' <---
                if (server.hasArg("brR_" + String(i))) preferences.putInt(("brR_" + String(i)).c_str(), server.arg("brR_" + String(i)).toInt());
                if (server.hasArg("brG_" + String(i))) preferences.putInt(("brG_" + String(i)).c_str(), server.arg("brG_" + String(i)).toInt());
                if (server.hasArg("brV_" + String(i))) preferences.putInt(("brV_" + String(i)).c_str(), server.arg("brV_" + String(i)).toInt());
            }
            preferences.end();
            server.send(200, "text/html", "<meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'><h2>💾 Mappatura e Luminosità Salvate!</h2>");
            delay(1000); ESP.restart();
        });
    }

  public:
    WifiConfigManager() : server(80), setupMode(false) {}

    // La funzione Callback aggiornata con l'INT della luminosità
    void setTestCallback(std::function<void(int, bool, int)> cb) { onTestPin = cb; }
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
        if (!setupMode && WiFi.status() == WL_CONNECTED) { ArduinoOTA.handle(); }
    }
    
    bool isSetupMode() const { return setupMode; }

    IPAddress getBrokerIP() {
        IPAddress ip;
        if (ip.fromString(mqtt_server_name)) { return ip; }
        String host = mqtt_server_name;
        if (host.endsWith(".local")) { host = host.substring(0, host.length() - 6); }
        return MDNS.queryHost(host);
    }
};

#endif