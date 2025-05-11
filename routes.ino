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

  // Envia também as últimas leituras para o box de sensores
  json += "],\"last_source\":" + String(tempSource, 2);
  json += ",\"last_target\":" + String(tempTarget, 2);
  json += ",\"last_cold\":" + String(tempCold, 2);

  json += "}";
  server.send(200, "application/json", json);
}



void handleRoot() {
  estimatedColdFlow = (coldPumpDuration / (float)coldPumpInterval) * theoreticalMaxColdFlow;
  float coldDuty = (coldPumpDuration / (float)coldPumpInterval) * 100.0;
  estimatedTargetFlow = (targetPumpDuration / (float)targetPumpInterval) * theoreticalMaxTargetFlow;
  float targetDuty = (targetPumpDuration / (float)targetPumpInterval) * 100.0;

  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body {font-family: Arial, sans-serif; margin:0; padding:10px; transition: background 0.3s, color 0.3s;}";
  html += "h1 {text-align:center; margin-bottom:20px;}";
  html += ".grid {display:grid; grid-template-columns:repeat(auto-fit,minmax(310px,1fr)); gap:15px;}";
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

  // Botões
  html += "<div class='header-buttons'>";
  html += "<button onclick=\"toggleMode()\">Toggle Mode</button>";
  html += "<button onclick=\"toggleSettings()\">Settings</button>";
  html += "</div>";

  html += "<h1>Heat Exchanger Controller</h1>";

  html += "<div class='grid'>";

  // Ajustes
  html += "<div class='card' id='settings' style='display:none;'><h2>Adjust Settings</h2><form action='/set'>";
  html += "<label>Cold Pump Pulse Interval (ms):</label><input name='interval' value='" + String(coldPumpInterval) + "'><br>";
  html += "<label>Cold Pump Pulse Duration (ms):</label><input name='duration' value='" + String(coldPumpDuration) + "'><br>";
  html += "<label>Target Recipe Pump Interval (ms):</label><input name='target_interval' value='" + String(targetPumpInterval) + "'><br>";
  html += "<label>Target Recipe Pump Duration (ms):</label><input name='target_duration' value='" + String(targetPumpDuration) + "'><br>";
  html += "<br><button type='submit'>Save</button></form>";
  html += "<button onclick=\"fetch('/reset_history').then(()=>alert('History cleared!'));\">Reset History</button></div>";

  // Sensores
  html += "<div class='card'><h2>Temperature Sensors</h2><p>";
  html += "Source Recipe: <b id='sourceTemp'>" + String(tempSource, 1) + " &deg;C</b><br>";
  html += "Target Recipe: <b id='targetTemp'>" + String(tempTarget, 1) + " &deg;C</b><br>";
  html += "Cold Recipe: <b id='coldTemp'>" + String(tempCold, 1) + " &deg;C</b></p></div>";

  // Status
  html += "<div class='card'><h2>Status</h2>";
  html += "<p>Water Level: <span class='badge " + String(levelDetected ? "yes'>YES" : "no'>NO") + "</span></p>";
  html += "<p>Heater: <span class='badge " + String(heaterStatus ? "yes'>ON" : "no'>OFF") + "</span></p></div>";

  // Cold Pump
  html += "<div class='card'><h2>Cold Recipe Pump</h2>";
  html += "<p>Interval: <b>" + String(coldPumpInterval) + " ms</b><br>";
  html += "Duration: <b>" + String(coldPumpDuration) + " ms</b><br>";
  html += "Duty: <b>" + String(coldDuty, 1) + "%</b><br>";
  html += "Flow: <b>" + String(estimatedColdFlow, 1) + " L/h</b></p></div>";

  // Target Pump
  html += "<div class='card'><h2>Target Recipe Pump</h2>";
  html += "<p>Interval: <b>" + String(targetPumpInterval) + " ms</b><br>";
  html += "Duration: <b>" + String(targetPumpDuration) + " ms</b><br>";
  html += "Duty: <b>" + String(targetDuty, 1) + "%</b><br>";
  html += "Flow: <b>" + String(estimatedTargetFlow, 1) + " L/h</b></p></div>";

  // Gráfico
  html += "<div class='card' style='grid-column: span 4;'><h2>Temperature Graph</h2>";
  html += "<canvas id='chart' style='max-height: 300px; width: 100%;'></canvas></div>";

  html += "</div>";

  // JS para toggle e gráfico + atualizações
  html += "<script>";
  html += "function toggleMode() { document.body.classList.toggle('light'); }";
  html += "function toggleSettings() { var s = document.getElementById('settings'); s.style.display = (s.style.display === 'none') ? 'block' : 'none'; }";

  // 👉 Variáveis do ESP32
  html += "const historySize = " + String(HISTORY_SIZE) + ";";
  html += "const lineTension = " + String(lineTension, 1) + ";";
  html += "const historyInterval = " + String(historyUpdateInterval / 1000) + ";";

  // 👉 Variáveis de configuração front-end
  html += "const animationEnabled = " + String(animationEnabled ? "true" : "false") + ";";
  html += "const sourceColor = 'red';";
  html += "const targetColor = 'blue';";
  html += "const coldColor = 'green';";
  html += "const sourceLabel = 'Source Recipe';";
  html += "const targetLabel = 'Target Recipe';";
  html += "const coldLabel = 'Cold Recipe';";
  html += "const efficiencyColor = 'orange';";
  html += "const efficiencyLabel = 'Efficiency %';";

  // 👉 Criação do gráfico
  html += "let ctx = document.getElementById('chart').getContext('2d');";
  html += "let chart = new Chart(ctx, {";
  html += "  type: 'line',";
  html += "  data: {";
  html += "    labels: [],";
  html += "    datasets: [";
  html += "      { label: sourceLabel, data: [], borderColor: sourceColor, fill: false, tension: lineTension, borderDash: [5, 5] },";
  html += "      { label: targetLabel, data: [], borderColor: targetColor, fill: false, tension: lineTension, borderDash: [5, 5] },";
  html += "      { label: coldLabel, data: [], borderColor: coldColor, fill: false, tension: lineTension, borderDash: [5, 5] },";
  html += "      { label: efficiencyLabel, data: [], borderColor: efficiencyColor, fill: false, tension: lineTension }";
  html += "    ]";
  html += "  },";
  html += "  options: {";
  html += "    animation: animationEnabled ? {} : { duration: 0 },";
  html += "    maintainAspectRatio: false,";
  html += "    scales: {";
  html += "      x: { title: { display: true, text: 'Time (s) since boot' } },";
  html += "      y: { title: { display: true, text: 'Temperature (\\u00B0C)' } }";
  html += "    }";
  html += "  }";
  html += "});";

  // 👉 Atualização automática
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