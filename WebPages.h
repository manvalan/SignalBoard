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
    body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 20px; background-color: #f4f4f9; color: #333;}
    h2 { color: #008CBA; }
    .container { background: white; padding: 20px; border-radius: 8px; box-shadow: 0px 0px 10px rgba(0,0,0,0.1); display: inline-block; text-align: left; max-width: 600px; width: 100%; margin-bottom: 20px;}
    input[type=text], input[type=password], input[type=number], select { width: 100%; padding: 10px; margin: 8px 0 15px 0; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
    input[type=submit], .btn { background-color: #4CAF50; color: white; padding: 12px 20px; border: none; border-radius: 4px; cursor: pointer; width: 100%; font-size: 16px; font-weight: bold; text-decoration: none; display: inline-block; text-align: center; box-sizing: border-box;}
    input[type=submit]:hover, .btn:hover { background-color: #45a049; }
    
    .nav { display: flex; flex-wrap: wrap; justify-content: center; gap: 5px; margin-bottom: 20px; }
    .nav a { text-decoration: none; padding: 10px 15px; background: #333; color: white; border-radius: 4px; font-size: 14px; font-weight: bold;}
    .nav a:hover { background: #555; }
    .nav a.active { background: #008CBA; }
    .nav a.danger { background: #f44336; }
    .nav a.danger:hover { background: #d32f2f; }
    
    .status-badge { background: #e7f3fe; color: #31708f; padding: 10px; border-left: 5px solid #2196F3; margin-bottom: 20px; border-radius: 4px; font-weight: bold;}
    
    /* Stili Dashboard & Segnali */
    .dash-card { background: #fff; padding: 15px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); margin-bottom: 15px; text-align: center; border-top: 4px solid #008CBA;}
    .btn-group { display: flex; gap: 10px; justify-content: center; margin-top: 10px; flex-wrap: wrap;}
    .btn-asp { padding: 10px 15px; border: none; border-radius: 4px; font-weight: bold; cursor: pointer; flex: 1; min-width: 80px; color: white; transition: 0.2s; }
    .btn-asp:hover { opacity: 0.8; }
    
    .legend { display: flex; justify-content: space-around; align-items: center; background: white; padding: 15px; border-radius: 8px; box-shadow: 0px 0px 10px rgba(0,0,0,0.1); max-width: 600px; margin: 0 auto 20px auto; }
    .legend-box { text-align: center; }
    .legend-box h4 { margin: 0 0 10px 0; color: #008CBA; font-size: 16px; border-bottom: 2px solid #eee; padding-bottom: 5px;}
    .signal-card { background: #f9f9f9; border-left: 5px solid #008CBA; padding: 15px; margin-bottom: 20px; border-radius: 4px; }
    .signal-card h3 { margin-top: 0; color: #008CBA; border-bottom: 1px solid #ddd; padding-bottom: 5px;}
    .pin-group { display: flex; gap: 10px; }
    .pin-group div { flex: 1; text-align: center; font-size: 13px; font-weight: bold; color: #444; background: #fff; padding: 10px; border-radius: 6px; border: 1px solid #ddd;}
    .test-btn { width: 100%; padding: 8px; border: none; border-radius: 4px; cursor: pointer; background-color: #e0e0e0; font-weight: bold; transition: 0.2s;}
  </style>
</head>
<body>
  <h2>Pannello di Controllo Wemos</h2>
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
  <div class="container" id="dash-container">
    <h3 style="text-align: center;">Caricamento stato...</h3>
  </div>

  <script>
    // Seleziona visivamente il tab attivo
    document.querySelector('a[href="/dashboard"]').classList.add('active');

    function caricaDashboard() {
      fetch('/api/status').then(r => r.json()).then(data => {
        let html = `<h3>Dashboard Segnali</h3>`;
        html += `<p style="text-align:center; font-size:14px; color:${data.mqtt_connected ? '#4CAF50' : '#F44336'};"><b>MQTT:</b> ${data.mqtt_connected ? 'Connesso' : 'Disconnesso'}</p>`;
        
        if (data.segnali.length === 0) {
          html += `<p style="text-align:center; color:#777;">Nessun segnale configurato. Vai nella sezione Hardware.</p>`;
        } else {
          data.segnali.forEach(s => {
            let tipoNome = s.tipo === 0 ? "Segnale Alto (Main)" : "Marmotta (Shunt)";
            html += `<div class="dash-card">
              <h4 style="margin:0 0 10px 0;">ID Rocrail: <span style="color:#008CBA;">${s.id}</span></h4>
              <p style="margin:0; font-size:12px; color:#666;">${tipoNome}</p>
              <div class="btn-group">`;
              
            if (s.tipo === 0) {
              html += `<button class="btn-asp" style="background:#F44336;" onclick="testLogico('${s.id}', 0)">ROSSO</button>`;
              html += `<button class="btn-asp" style="background:#4CAF50;" onclick="testLogico('${s.id}', 1)">VERDE</button>`;
              html += `<button class="btn-asp" style="background:#FFC107; color:#333;" onclick="testLogico('${s.id}', 2)">GIALLO</button>`;
            } else {
              html += `<button class="btn-asp" style="background:#F44336;" onclick="testLogico('${s.id}', 3)">STOP</button>`;
              html += `<button class="btn-asp" style="background:#4CAF50;" onclick="testLogico('${s.id}', 4)">AVANZA</button>`;
              html += `<button class="btn-asp" style="background:#2196F3;" onclick="testLogico('${s.id}', 5)">OBLIQUO</button>`;
            }
            html += `</div></div>`;
          });
        }
        document.getElementById('dash-container').innerHTML = html;
      });
    }

    function testLogico(id, aspetto) {
      fetch(`/test_signal?id=${id}&aspect=${aspetto}`)
        .then(r => { if(!r.ok) alert("Errore di invio comando"); });
    }

    caricaDashboard();
    setInterval(caricaDashboard, 5000);
  </script>
</body></html>
)rawliteral";

const char system_html[] PROGMEM = R"rawliteral(
  <div class="status-badge">
    📡 Stato Wi-Fi: %WIFI_STATUS%
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
      <p style="font-size: 12px; color: #777;">La scheda sarà raggiungibile all'indirizzo: <strong>http://[nome-scheda].local</strong></p>
      
      <label>Nuova Password di Accesso (Web e OTA):</label>
      <input type="password" name="web_pass" placeholder="Lascia vuoto per non cambiare">
      <p style="font-size: 12px; color: #777;">Utente predefinito: <strong>admin</strong></p>
      
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
      <label>Server MQTT (IP o Nome):</label>
      <input type="text" name="mqtt_server" placeholder="es. plastico o 192.168.1.X" required value=")rawliteral";

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
    <p>Autore: Michele Bigi</p>
  </div>
  <script>document.querySelector('a[href="/info"]').classList.add('active');</script>
</body></html>
)rawliteral";

const char mapping_script_and_legend[] PROGMEM = R"rawliteral(
  <script>
    document.querySelector('a[href="/mapping"]').classList.add('active');
    function testPin(inputId, btn, colorHex) {
      var pinVal = document.querySelector('input[name="' + inputId + '"]').value;
      if(pinVal === "") { alert("Inserisci un canale."); return; }
      var state = btn.classList.contains('active') ? 0 : 1; 
      fetch('/test_pin?pin=' + pinVal + '&state=' + state).then(response => {
        if(response.ok) {
          if(state === 1) {
            btn.classList.add('active'); btn.style.backgroundColor = colorHex; btn.style.color = 'white'; btn.innerText = 'SPEGNI';
          } else {
            btn.classList.remove('active'); btn.style.backgroundColor = '#e0e0e0'; btn.style.color = 'black'; btn.innerText = 'TEST';
          }
        }
      });
    }
  </script>
  <div class="legend">
    <div class="legend-box">
      <h4>Segnale Alto (Main)</h4>
      <div style="text-align: left; font-size: 14px; line-height: 1.8;">🔴 Rosso (Alto)<br>🟡 Giallo (Centro)<br>🟢 Verde (Basso)</div>
    </div>
    <div class="legend-box">
      <h4>Marmotta (Shunt)</h4>
      <svg viewBox="0 0 100 100" style="width: 70px; height: 70px; background: #2b2b2b; border-radius: 8px; padding: 8px; margin-top: 5px;">
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