// Routes
void handleData() {
  String json = "{";

  json += "\"source\":[";
  for (int i = 0; i < 10; i++) {
    json += String(sourceTempHistory[i], 2);
    if (i < 9) json += ",";
  }

  json += "],\"target\":[";
  for (int i = 0; i < 10; i++) {
    json += String(targetTempHistory[i], 2);
    if (i < 9) json += ",";
  }

  json += "],\"cold\":[";
  for (int i = 0; i < 10; i++) {
    json += String(coldTempHistory[i], 2);
    if (i < 9) json += ",";
  }

  json += "],\"efficiency\":[";
  for (int i = 0; i < 10; i++) {
    json += String(efficiencyHistory[i], 2);
    if (i < 9) json += ",";
  }

  json += "],\"last_source\":" + String(tempSource, 2);
  json += ",\"last_target\":" + String(tempTarget, 2);
  json += ",\"last_cold\":" + String(tempCold, 2);

  // Novos campos para atualização do Status
  json += ",\"heater\":" + String(digitalRead(HEATER_PIN) ? "1" : "0");
  json += ",\"cold_pump\":" + String(digitalRead(COLD_PUMP_PIN) ? "1" : "0");
  json += ",\"target_pump\":" + String(digitalRead(TARGET_PUMP_PIN) ? "1" : "0");
  json += ",\"valve\":" + String(digitalRead(VALVE_PIN) ? "1" : "0");
  json += ",\"mode\":" + String(manualMode ? "1" : "0");

  json += "}";
  server.send(200, "application/json", json);
}



void handleRoot() {
  // Formulas
  //

  // Main HTML
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body {font-family: Arial, sans-serif; margin:0; padding:10px; transition: background 0.3s, color 0.3s;}";
  html += "h1 {text-align:center; margin-bottom:20px;}";
  html += ".grid {display:grid; grid-template-columns:repeat(auto-fit,minmax(310px,1fr)); gap:15px;}";
  html += ".header-buttons { display: flex; justify-content: flex-end; gap: 10px; margin-bottom: 10px; }";
  html += "table { width: 100%; border-collapse: collapse; font-size: 1em; }";
  html += "table, td, th, tr { color: var(--text-color); }";
  html += "td, th { padding: 8px 12px; text-align: left; }";
  html += "tr:nth-child(even) { background-color: rgba(0,0,0,0.05); }";
  html += "tr:hover { background-color: rgba(0,0,0,0.1); }";
  html += ".card {padding:15px; border-radius:10px; box-shadow:0 4px 8px rgba(0,0,0,0.2); background: var(--card-bg); color: var(--text-color);}";
  html += ".badge {padding:4px 8px; border-radius:5px; font-size:0.9em;}";
  html += ".yes {background:#27ae60; color:white;}";
  html += ".no {background:#c0392b; color:white;}";
  html += "input, button {border:none; border-radius:5px; padding:6px; margin-top:5px;}";
  html += "input {width:80px; border:1px solid #999; background-color:#fff;}";
  html += "label {display:inline-block; width:160px;}";
  html += "canvas {background:#fff; border-radius:10px;}";
  html += ":root {--bg-color:#ecf0f1; --text-color:#2c3e50; --card-bg:#ffffff;}";
  html += "body.light {--bg-color:#2c3e50; --text-color:#ecf0f1; --card-bg:#34495e;}";
  html += "@media (max-width: 768px) {.grid {} .card {grid-column: span 1 !important;}}";
  html += "</style>";
  html += "<script src=\"/chart.min.js\"></script></head>";
  html += "<body style='background: var(--bg-color); color: var(--text-color);'>";

  // Header Buttons
  html += "<div class='header-buttons'>";
  html += "<button onclick=\"toggleMode()\">Toggle Mode</button>";
  html += "<button onclick=\"toggleSettings()\">Settings</button>";
  html += "<button onclick=\"toggleManual()\">Manual</button>";
  html += "</div>";

  html += "<h1>Heat Exchanger Controller</h1>";

  html += "<div class='grid'>";

  // Status
  html += "<div class='card'>";
  html += "<h2>Temperature Sensors</h2><p>";
  html += "Source Recipe: <b id='sourceTemp'>" + String(tempSource, 1) + " &deg;C</b><br>";
  html += "Target Recipe: <b id='targetTemp'>" + String(tempTarget, 1) + " &deg;C</b><br>";
  html += "Cold Recipe: <b id='coldTemp'>" + String(tempCold, 1) + " &deg;C</b></p>";
  html += "<h2>System Status</h2>";
  html += "<table style='width:100%; font-size:1em;'>";
  html += "<tr><td>Heater</td><td><span id='status-heater' class='badge " + String(digitalRead(HEATER_PIN) ? "yes'>ON" : "no'>OFF") + "</span></td></tr>";
  html += "<tr><td>Cold Pump</td><td><span id='status-cold' class='badge " + String(digitalRead(COLD_PUMP_PIN) ? "yes'>ON" : "no'>OFF") + "</span></td></tr>";
  html += "<tr><td>Target Pump</td><td><span id='status-target' class='badge " + String(digitalRead(TARGET_PUMP_PIN) ? "yes'>ON" : "no'>OFF") + "</span></td></tr>";
  html += "<tr><td>Valve</td><td><span id='status-valve' class='badge " + String(digitalRead(VALVE_PIN) ? "yes'>OPEN" : "no'>CLOSED") + "</span></td></tr>";
  html += "<tr><td>Mode</td><td><span id='status-mode' class='badge " + String(manualMode ? "no'>MANUAL" : "yes'>AUTO") + "</span></td></tr>";
  html += "</table></div>";

  // Pumps Info 
  html += "<div class='card'><h2>Pumps Info</h2>";

  // 👉 Cold Pump
  float coldDuty = (coldPumpInterval > 0) ? ((float)coldPumpDuration / (float)coldPumpInterval) * 100.0 : 100.0;
  float coldEffectiveFlow = (coldPumpInterval > 0) ? theoreticalMaxColdFlow * ((float)coldPumpDuration / (float)coldPumpInterval) : theoreticalMaxColdFlow;
  float coldTimeFor5L = (coldEffectiveFlow > 0) ? (5.0 / coldEffectiveFlow) * 60.0 : 0;
  String coldTimeStr = (coldTimeFor5L < 1.0) ? String(coldTimeFor5L * 60.0, 0) + " sec" : String(coldTimeFor5L, 1) + " min";

  html += "<h3 style='margin-bottom:5px;'>Cold Pump</h3>";
  html += "<p>Mode: <b>" + String((coldPumpInterval > 0 && coldPumpDuration > 0) ? "Pulsed" : "Continuous") + "</b><br>";
  html += "Flow Rate: <b>" + String(coldEffectiveFlow, 1) + " L/h</b><br>";
  html += "Duty Cycle: <b>" + String(coldDuty, 1) + " %</b><br>";
  if (coldPumpInterval > 0 && coldPumpDuration > 0) {
    float cyclesPerHour = 3600000.0 / coldPumpInterval;
    float volumePerCycle = theoreticalMaxColdFlow * (coldPumpDuration / 3600000.0);
    html += "Volume per Cycle: <b>" + String(volumePerCycle, 3) + " L</b><br>";
    html += "Cycles per Hour: <b>" + String(cyclesPerHour, 0) + "</b><br>";
  }
  html += "Est. Time to recirculate 5L: <b>" + coldTimeStr + "</b></p>";

  // 👉 Target Pump
  float targetDuty = (targetPumpInterval > 0) ? ((float)targetPumpDuration / (float)targetPumpInterval) * 100.0 : 100.0;
  float targetEffectiveFlow = (targetPumpInterval > 0) ? theoreticalMaxTargetFlow * ((float)targetPumpDuration / (float)targetPumpInterval) : theoreticalMaxTargetFlow;
  float targetTimeFor5L = (targetEffectiveFlow > 0) ? (5.0 / targetEffectiveFlow) * 60.0 : 0;
  String targetTimeStr = (targetTimeFor5L < 1.0) ? String(targetTimeFor5L * 60.0, 0) + " sec" : String(targetTimeFor5L, 1) + " min";

  html += "<h3 style='margin-bottom:5px;'>Target Pump</h3>";
  html += "<p>Mode: <b>" + String((targetPumpInterval > 0 && targetPumpDuration > 0) ? "Pulsed" : "Continuous") + "</b><br>";
  html += "Flow Rate: <b>" + String(targetEffectiveFlow, 1) + " L/h</b><br>";
  html += "Duty Cycle: <b>" + String(targetDuty, 1) + " %</b><br>";
  html += "Est. Time to recirculate 5L: <b>" + targetTimeStr + "</b></p>";

  html += "</div>";

  // Settings
  html += "<div class='card' id='settings' style='display:none;'><h2>Adjust Settings</h2><form action='/set'>";
  html += "<label>Cold Pump Pulse Interval (ms):</label><input name='interval' value='" + String(coldPumpInterval) + "'><br>";
  html += "<label>Cold Pump Pulse Duration (ms):</label><input name='duration' value='" + String(coldPumpDuration) + "'><br>";
  html += "<label>Target Recipe Pump Interval (ms):</label><input name='target_interval' value='" + String(targetPumpInterval) + "'><br>";
  html += "<label>Target Recipe Pump Duration (ms):</label><input name='target_duration' value='" + String(targetPumpDuration) + "'><br>";
  html += "<br><button type='submit'>Save</button></form>";
  html += "<button onclick=\"fetch('/reset_history').then(()=>alert('History cleared!'));\">Reset History</button></div>";

  // Manual control
  html += "<div class='card' id='manual' style='display:none;'><h2>Manual Control</h2>";
  html += "<button id='returnAuto' style='width:100%;background:#2ecc71;color:white;' onclick=\"fetch('/manual/off',{cache:'no-cache'}).then(() => { this.innerText = 'SWITCHING...'; this.disabled = true; })\">RETURN TO AUTO MODE</button>";
  html += "<table>";
  html += "<tr><td>Heater</td><td>";
  html += "<button onclick=\"fetch('/manual/heater/on',{cache:'no-cache'})\">ON</button> ";
  html += "<button onclick=\"fetch('/manual/heater/off',{cache:'no-cache'})\">OFF</button>";
  html += "</td></tr>";
  html += "<tr><td>Valve</td><td>";
  html += "<button onclick=\"fetch('/manual/valve/on',{cache:'no-cache'})\">OPEN</button> ";
  html += "<button onclick=\"fetch('/manual/valve/off',{cache:'no-cache'})\">CLOSE</button>";
  html += "</td></tr>";
  html += "<tr><td>Cold Pump</td><td>";
  html += "<button onclick=\"fetch('/manual/cold/on',{cache:'no-cache'})\">ON</button> ";
  html += "<button onclick=\"fetch('/manual/cold/off',{cache:'no-cache'})\">OFF</button>";
  html += "</td></tr>";
  html += "<tr><td>Target Pump</td><td>";
  html += "<button onclick=\"fetch('/manual/target/on',{cache:'no-cache'})\">ON</button> ";
  html += "<button onclick=\"fetch('/manual/target/off',{cache:'no-cache'})\">OFF</button>";
  html += "</td></tr>";
  html += "</table></div>";


  // Chart Box
  html += "<div class='card' style='grid-column: span 4;'><h2>Temperature Graph</h2>";
  html += "<canvas id='chart' style='max-height: 300px; width: 100%;'></canvas></div>";

  html += "</div>";

  // JS to toggle, charts and updates
  html += "<script>";
  html += "function toggleMode() { document.body.classList.toggle('light'); }";
  html += "function toggleSettings() { var s = document.getElementById('settings'); s.style.display = (s.style.display === 'none') ? 'block' : 'none'; }";
  html += "function toggleManual() { var s = document.getElementById('manual'); s.style.display = (s.style.display === 'none') ? 'block' : 'none'; }";

  // ESP32 Variables
  html += "const historySize = " + String(HISTORY_SIZE) + ";";
  html += "const lineTension = " + String(lineTension, 1) + ";";
  html += "const historyInterval = " + String(historyUpdateInterval / 1000) + ";";

  // Front-End configuration
  html += "const animationEnabled = " + String(animationEnabled ? "true" : "false") + ";";
  html += "const sourceColor = 'red';";
  html += "const targetColor = 'orange';";
  html += "const coldColor = 'blue';";
  html += "const sourceLabel = 'Source Recipe';";
  html += "const targetLabel = 'Target Recipe';";
  html += "const coldLabel = 'Cold Recipe';";
  html += "const efficiencyColor = 'green';";
  html += "const efficiencyLabel = 'Efficiency %';";

  // Chart Creation
  html += "let ctx = document.getElementById('chart').getContext('2d');";
  html += "let chart = new Chart(ctx, {";
  html += "  type: 'line',";
  html += "  data: {";
  html += "    labels: [],";
  html += "    datasets: [";
  html += "      { label: sourceLabel, data: [], borderColor: sourceColor, fill: false, tension: lineTension, borderDash: [5, 5], yAxisID: 'y' },";
  html += "      { label: targetLabel, data: [], borderColor: targetColor, fill: false, tension: lineTension, borderDash: [5, 5], yAxisID: 'y' },";
  html += "      { label: coldLabel, data: [], borderColor: coldColor, fill: false, tension: lineTension, borderDash: [5, 5], yAxisID: 'y' },";
  html += "      { label: efficiencyLabel, data: [], borderColor: efficiencyColor, fill: false, tension: lineTension, yAxisID: 'y1' }";
  html += "    ]";
  html += "  },";
  html += "  options: {";
  html += "    animation: animationEnabled ? {} : { duration: 0 },";
  html += "    maintainAspectRatio: false,";
  html += "    scales: {";
  html += "      x: { title: { display: true, text: 'Time (s) since boot' } },";
  html += "      y: { title: { display: true, text: 'Temperature (\\u00B0C)' }, position: 'left' },";
  html += "      y1: { title: { display: true, text: 'Efficiency (%)' }, position: 'right', grid: { drawOnChartArea: false } }";
  html += "    }";
  html += "  }";
  html += "});";

  // Automatic Updates
  html += "setInterval(() => {";
  html += "  fetch('/data').then(r => r.json()).then(d => {";
  html += "    chart.data.labels = [...Array(historySize).keys()].map(i => i * historyInterval);";
  html += "    chart.data.datasets[0].data = d.source;";
  html += "    chart.data.datasets[1].data = d.target;";
  html += "    chart.data.datasets[2].data = d.cold;";
  html += "    chart.data.datasets[3].data = d.efficiency;";
  html += "    chart.update();";
  html += "    document.getElementById('sourceTemp').innerHTML = d.last_source.toFixed(1) + ' \\u00B0C';";
  html += "    document.getElementById('targetTemp').innerHTML = d.last_target.toFixed(1) + ' \\u00B0C';";
  html += "    document.getElementById('coldTemp').innerHTML = d.last_cold.toFixed(1) + ' \\u00B0C';";

  html += "    document.getElementById('status-heater').className = 'badge ' + (d.heater ? 'yes' : 'no');";
  html += "    document.getElementById('status-heater').innerText = d.heater ? 'ON' : 'OFF';";

  html += "    document.getElementById('status-cold').className = 'badge ' + (d.cold_pump ? 'yes' : 'no');";
  html += "    document.getElementById('status-cold').innerText = d.cold_pump ? 'ON' : 'OFF';";

  html += "    document.getElementById('status-target').className = 'badge ' + (d.target_pump ? 'yes' : 'no');";
  html += "    document.getElementById('status-target').innerText = d.target_pump ? 'ON' : 'OFF';";

  html += "    document.getElementById('status-valve').className = 'badge ' + (d.valve ? 'yes' : 'no');";
  html += "    document.getElementById('status-valve').innerText = d.valve ? 'OPEN' : 'CLOSED';";

  html += "    document.getElementById('status-mode').className = 'badge ' + (d.mode ? 'no' : 'yes');";
  html += "    document.getElementById('status-mode').innerText = d.mode ? 'MANUAL' : 'AUTO';";

  html += "    document.getElementById('returnAuto').disabled = !d.mode;";
  html += "    document.getElementById('returnAuto').innerText = 'RETURN TO AUTO MODE';";

  html += "  });";
  html += "}, 5000);";



  html += "</script></body></html>";

  server.send(200, "text/html", html);
}

void handleSet() {
  if (server.hasArg("interval")) coldPumpInterval = server.arg("interval").toInt();
  if (server.hasArg("duration")) coldPumpDuration = server.arg("duration").toInt();
  if (server.hasArg("target_interval")) targetPumpInterval = server.arg("target_interval").toInt();
  if (server.hasArg("target_duration")) targetPumpDuration = server.arg("target_duration").toInt();
  server.sendHeader("Location", "/");
  server.send(303);
}