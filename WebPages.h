#ifndef WEBPAGES_H
#define WEBPAGES_H

#include <Arduino.h>

const char html_header[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8">
  <title>Centrale Segnali - Setup</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    /* Variabili per Tema Chiaro / Scuro */
    :root {
      --bg-color: #e9ecef;
      --text-color: #333;
      --card-bg: rgba(255, 255, 255, 0.85);
      --card-border: rgba(255, 255, 255, 0.4);
      --shadow-color: rgba(0, 0, 0, 0.1);
      --glass-blur: blur(12px);
      --primary-color: #008CBA;
      --nav-bg: rgba(50, 50, 50, 0.8);
      --nav-text: #fff;
    }

    /* Modalità Scura Automatica */
    @media (prefers-color-scheme: dark) {
      :root {
        --bg-color: #121212;
        --text-color: #e0e0e0;
        --card-bg: rgba(30, 30, 30, 0.75);
        --card-border: rgba(255, 255, 255, 0.1);
        --shadow-color: rgba(0, 0, 0, 0.4);
        --primary-color: #4fc3f7;
        --nav-bg: rgba(20, 20, 20, 0.8);
      }
    }

    body { 
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; 
      text-align: center; margin: 0; padding: 20px; 
      background-color: var(--bg-color); color: var(--text-color);
      transition: background-color 0.3s, color 0.3s;
    }
    
    h2, h3, h4 { color: var(--primary-color); }
    
    /* Effetto Liquid Glass */
    .container, .dash-card, .legend, .signal-card { 
      background: var(--card-bg); 
      backdrop-filter: var(--glass-blur);
      -webkit-backdrop-filter: var(--glass-blur);
      border: 1px solid var(--card-border);
      border-radius: 12px; 
      box-shadow: 0 8px 32px var(--shadow-color); 
      padding: 20px; margin-bottom: 20px; 
      display: inline-block; text-align: left; max-width: 600px; width: 100%; box-sizing: border-box;
    }
    
    input[type=text], input[type=password], input[type=number], select { 
      width: 100%; padding: 10px; margin: 8px 0 15px 0; 
      border: 1px solid var(--card-border); border-radius: 6px; box-sizing: border-box; 
      background: var(--card-bg); color: var(--text-color); font-family: inherit;
    }
    
    input[type=submit], .btn { 
      background-color: #4CAF50; color: white; padding: 12px 20px; 
      border: none; border-radius: 6px; cursor: pointer; width: 100%; 
      font-size: 16px; font-weight: bold; transition: 0.2s;
    }
    input[type=submit]:hover, .btn:hover { opacity: 0.9; transform: translateY(-2px); }
    
    /* Menu Navigazione */
    .nav { display: flex; flex-wrap: wrap; justify-content: center; gap: 8px; margin-bottom: 25px; }
    .nav a { 
      text-decoration: none; padding: 10px 15px; 
      background: var(--nav-bg); color: var(--nav-text); 
      border-radius: 8px; font-size: 14px; font-weight: bold; transition: 0.2s;
      backdrop-filter: blur(4px); -webkit-backdrop-filter: blur(4px);
    }
    .nav a:hover { opacity: 0.8; }
    .nav a.active { background: var(--primary-color); box-shadow: 0 0 10px rgba(0,140,186,0.5); }
    .nav a.danger { background: #d32f2f; }
    
    /* === NUOVI STILI BOTTONI === */
    .btn-group { display: flex; gap: 8px; justify-content: center; margin-top: 15px; flex-wrap: wrap;}
    
    /* Base comune per tutti i pulsanti aspetto */
    .btn-asp { 
      border: none; font-weight: bold; cursor: pointer; flex: 1; 
      transition: all 0.3s ease; opacity: 0.4; transform: scale(0.95);
    }
    .btn-asp:hover { opacity: 0.7; }
    
    /* Stile specifico: Segnale Alto (Main) - Squadrato e compatto */
    .btn-main {
      padding: 10px 12px; border-radius: 6px; font-size: 13px; min-width: 70px;
    }

    /* Stile specifico: Marmotta (Shunt) - Tondo a pillola e piccolo */
    .btn-shunt {
      padding: 6px 10px; border-radius: 20px; font-size: 11px; min-width: 60px; letter-spacing: 0.5px;
    }
    
    /* Animazione e bagliore quando il segnale è attivo */
    .active-btn { 
      opacity: 1 !important; 
      transform: scale(1.05) !important; 
      box-shadow: 0 0 12px currentColor; 
      z-index: 2;
    }

    .pin-group { display: flex; gap: 10px; }
    .pin-group div { flex: 1; text-align: center; font-size: 13px; font-weight: bold; }
    .test-btn { width: 100%; padding: 8px; margin-top: 5px; border: none; border-radius: 4px; cursor: pointer; background-color: #555; color: white; transition: 0.2s;}
  </style>
</head>
<body>
  <h2>Pannello di Controllo</h2>
  <div class="nav">
    <a href="/">Sistema</a>
    <a href="/mqtt">MQTT</a>
    <a href="/mapping">Hardware</a>
    <a href="/dashboard">Dashboard</a>
    <a href="/info">Info</a>
    <a href="/hard_reset" class="danger" onclick="return confirm('Cancellare TUTTO e ripristinare il Wemos?');">Hard Reset</a>
  </div>
)rawliteral";

const char dashboard_html[] PROGMEM = R"rawliteral(
  <div class="container" id="dash-container" style="text-align: center;">
    <h3>Caricamento stato...</h3>
  </div>

  <script>
    document.querySelector('a[href="/dashboard"]').classList.add('active');

    function formatUptime(seconds) {
        let h = Math.floor(seconds / 3600);
        let m = Math.floor((seconds % 3600) / 60);
        let s = seconds % 60;
        return `${h}h ${m}m ${s}s`;
    }

    function caricaDashboard() {
      fetch('/api/status').then(r => r.json()).then(data => {
        let html = ``;

        let mqttColor = data.mqtt_connected ? '#4CAF50' : '#F44336';
        let mqttText = data.mqtt_connected ? 'CONNESSO' : 'DISCONNESSO';
        
        html += `<div style="display:flex; justify-content:space-around; flex-wrap:wrap; background:rgba(0,0,0,0.05); padding:15px; border-radius:8px; margin-bottom:25px; border-left: 5px solid ${mqttColor};">
          <div style="margin:5px 10px;"><b>📡 Wi-Fi:</b><br>${data.rssi} dBm<br><span style="font-size:11px; opacity:0.7;">IP: ${data.ip}</span></div>
          <div style="margin:5px 10px;"><b>⏱️ Uptime:</b><br>${formatUptime(data.uptime)}</div>
          <div style="margin:5px 10px; font-size: 15px; font-weight:bold; color:${mqttColor};"><b>🔌 MQTT:</b><br>${mqttText}</div>
        </div>`;
        
        if (data.segnali.length === 0) {
          html += `<p style="opacity:0.7;">Nessun segnale configurato. Vai nella sezione Hardware.</p>`;
        } else {
          data.segnali.forEach(s => {
            let tipoNome = s.tipo === 0 ? "Segnale Alto (Main)" : "Marmotta (Shunt)";
            html += `<div class="dash-card" style="width: 100%; box-sizing: border-box; text-align: center; padding: 15px;">
              <h4 style="margin:0 0 5px 0;">ID Rocrail: <span style="color:var(--primary-color);">${s.id}</span></h4>
              <p style="margin:0 0 10px 0; font-size:12px; opacity:0.7;">${tipoNome}</p>
              <div class="btn-group">`;
              
            if (s.tipo === 0) {
              // Applica classe btn-main ai segnali alti
              html += `<button class="btn-asp btn-main ${s.aspetto === 0 ? 'active-btn' : ''}" style="background:#F44336; color:white;" onclick="testLogico('${s.id}', 0)">ROSSO</button>`;
              html += `<button class="btn-asp btn-main ${s.aspetto === 1 ? 'active-btn' : ''}" style="background:#4CAF50; color:white;" onclick="testLogico('${s.id}', 1)">VERDE</button>`;
              html += `<button class="btn-asp btn-main ${s.aspetto === 2 ? 'active-btn' : ''}" style="background:#FFC107; color:#333;" onclick="testLogico('${s.id}', 2)">GIALLO</button>`;
            } else {
              // Applica classe btn-shunt alle marmotte
              html += `<button class="btn-asp btn-shunt ${s.aspetto === 3 ? 'active-btn' : ''}" style="background:#F44336; color:white;" onclick="testLogico('${s.id}', 3)">STOP</button>`;
              html += `<button class="btn-asp btn-shunt ${s.aspetto === 4 ? 'active-btn' : ''}" style="background:#4CAF50; color:white;" onclick="testLogico('${s.id}', 4)">AVANZA</button>`;
              html += `<button class="btn-asp btn-shunt ${s.aspetto === 5 ? 'active-btn' : ''}" style="background:#2196F3; color:white;" onclick="testLogico('${s.id}', 5)">OBLIQUO</button>`;
            }
            html += `</div></div>`;
          });
        }
        document.getElementById('dash-container').innerHTML = html;
      });
    }

    function testLogico(id, aspetto) {
      fetch(`/test_signal?id=${id}&aspect=${aspetto}`)
        .then(r => { 
            if(!r.ok) alert("Errore di invio comando");
            caricaDashboard(); 
        });
    }

    caricaDashboard();
    setInterval(caricaDashboard, 4000); 
  </script>
</body></html>
)rawliteral";

const char system_html[] PROGMEM = R"rawliteral(
  <div class="container" style="text-align: center; background: rgba(33, 150, 243, 0.1); border-left: 5px solid #2196F3;">
    <b>📡 Stato Wi-Fi:</b> %WIFI_STATUS%
  </div>

  <div class="container">
    <h3>Configurazione Rete Wi-Fi</h3>
    <form action="/save_wifi" method="POST">
      <label>Seleziona Rete Wi-Fi (SSID):</label>
      <select name="ssid" id="ssid_select"><option value="">Scansione reti in corso...</option></select>
      <label>Oppure manuale:</label>
      <input type="text" name="ssid_manual" placeholder="Nome della rete nascosta">
      <label>Password Wi-Fi:</label>
      <input type="password" name="password" placeholder="La tua password">
      <input type="submit" value="💾 Salva Wi-Fi">
    </form>
  </div>

  <div class="container">
    <h3>Identità e Sicurezza</h3>
    <form action="/save_security" method="POST">
      <label>Nome Scheda (mDNS e OTA):</label>
      <input type="text" name="hostname" value="%HOSTNAME%" required>
      <p style="font-size: 12px; opacity: 0.7;">Indirizzo locale: <strong>http://[nome-scheda].local</strong></p>
      
      <label>Nuova Password di Accesso:</label>
      <input type="password" name="web_pass" placeholder="Lascia vuoto per non cambiare">
      
      <input type="submit" value="🛡️ Salva Sicurezza">
    </form>
  </div>
  
  <script>
    document.querySelector('a[href="/"]').classList.add('active');
    fetch('/scan_wifi').then(response => response.text()).then(html => {
      document.getElementById('ssid_select').innerHTML = html;
    });
  </script>
</body></html>
)rawliteral";

const char mqtt_html_top[] PROGMEM = R"rawliteral(
  <div class="container">
    <h3>Configurazione Broker MQTT</h3>
    <form action="/save_mqtt" method="POST">
      <label>Server MQTT (IP numerico o nome .local):</label>
      <input type="text" name="mqtt_server" placeholder="es. plastico.local o 192.168.1.X" required value=")rawliteral";

const char mqtt_html_bottom[] PROGMEM = R"rawliteral(">
      <input type="submit" value="💾 Salva MQTT">
    </form>
  </div>
  <script>document.querySelector('a[href="/mqtt"]').classList.add('active');</script>
</body></html>
)rawliteral";

const char info_html[] PROGMEM = R"rawliteral(
  <div class="container" style="text-align: center;">
    <h3>Informazioni Sistema</h3>
    <p><strong>Firmware Segnali Rocrail</strong></p>
    <p>Sviluppato per ESP32 & PCA9685</p>
    <p style="opacity: 0.6; font-size: 12px;">Autore: Michele Bigi</p>
  </div>
  <script>document.querySelector('a[href="/info"]').classList.add('active');</script>
</body></html>
)rawliteral";

onst char mapping_script_and_legend[] PROGMEM = R"rawliteral(
  <script>
    document.querySelector('a[href="/mapping"]').classList.add('active');
    
    // Funzione aggiornata per inviare anche il livello di luminosità (brId)
    function testPin(inputId, brId, btn, colorHex) {
      var pinVal = document.querySelector('input[name="' + inputId + '"]').value;
      var brVal = document.querySelector('input[name="' + brId + '"]').value; // Legge lo slider
      
      if(pinVal === "") { alert("Inserisci un canale."); return; }
      var state = btn.classList.contains('active') ? 0 : 1; 
      
      // Invia il comando all'API includendo &br=
      fetch('/test_pin?pin=' + pinVal + '&state=' + state + '&br=' + brVal).then(response => {
        if(response.ok) {
          if(state === 1) {
            btn.classList.add('active'); btn.style.backgroundColor = colorHex; btn.style.color = 'white'; btn.innerText = 'SPEGNI';
          } else {
            btn.classList.remove('active'); btn.style.backgroundColor = '#555'; btn.style.color = 'white'; btn.innerText = 'TEST';
          }
        }
      });
    }
  </script>
  <style>
    /* Stile per fare i range slider più belli */
    input[type=range] { width: 90%; margin: 8px 0; cursor: pointer; accent-color: var(--primary-color); }
  </style>
  <div class="legend">
    <div style="flex: 1; text-align: center;">
      <h4 style="margin-top:0;">Main (Alto)</h4>
      <div style="font-size: 13px; line-height: 1.8; text-align: left; display: inline-block;">🔴 Rosso (Alto)<br>🟡 Giallo (Centro)<br>🟢 Verde (Basso)</div>
    </div>
    <div style="flex: 1; text-align: center;">
      <h4 style="margin-top:0;">Shunt (Marmotta)</h4>
      <svg viewBox="0 0 100 100" style="width: 60px; height: 60px; background: #2b2b2b; border-radius: 8px; padding: 5px;">
        <circle cx="30" cy="30" r="14" fill="#ffffff" stroke="#555" stroke-width="2"/>
        <text x="30" y="35" fill="#000" font-size="14" font-weight="bold" text-anchor="middle" font-family="sans-serif">C</text>
        <circle cx="30" cy="70" r="14" fill="#ffffff" stroke="#555" stroke-width="2"/>
        <text x="30" y="75" fill="#000" font-size="14" font-weight="bold" text-anchor="middle" font-family="sans-serif">B</text>
        <circle cx="70" cy="70" r="14" fill="#ffffff" stroke="#555" stroke-width="2"/>
        <text x="70" y="75" fill="#000" font-size="14" font-weight="bold" text-anchor="middle" font-family="sans-serif">A</text>
      </svg>
    </div>
  </div>
  <form action="/save_mapping" method="POST">
)rawliteral";

#endif
