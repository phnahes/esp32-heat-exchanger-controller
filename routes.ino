// Routes
void configRoutes() {
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/set", handleSet);

  server.on("/chart.min.js", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "public, max-age=31536000");
    server.send_P(200, "application/javascript", chartJs);
  });

  server.on("/reset_history", []() {
    clearHistory();
    maxTempSource = 0.0;
    server.send(200, "text/plain", "History reset");
  });

  // Rotas manual mode
  server.on("/manual/on", []() {
    manualMode = true;
    server.send(200, "text/plain", "Manual ON");
  });

  server.on("/manual/off", []() {
    manualMode = false;
    manualColdPumpActive = manualTargetPumpActive = manualHeaterActive = false;
    server.send(200, "text/plain", "Manual OFF");
  });

  server.on("/manual/cold/on", []() {
    manualMode = true;
    manualColdPumpActive = true;
    server.send(200, "text/plain", "Cold Pump ON");
  });

  server.on("/manual/cold/off", []() {
    manualMode = true;
    manualColdPumpActive = false;
    server.send(200, "text/plain", "Cold Pump OFF");
  });

  server.on("/manual/target/on", []() {
    manualMode = true;
    manualTargetPumpActive = true;
    server.send(200, "text/plain", "Target Pump ON");
  });

  server.on("/manual/target/off", []() {
    manualMode = true;
    manualTargetPumpActive = false;
    server.send(200, "text/plain", "Target Pump OFF");
  });

  server.on("/manual/heater/on", []() {
    manualMode = true;
    manualHeaterActive = true;
    server.send(200, "text/plain", "Heater ON");
  });

  server.on("/manual/heater/off", []() {
    manualMode = true;
    manualHeaterActive = false;
    server.send(200, "text/plain", "Heater OFF");
  });
  server.on("/favicon.ico", []() {
    server.send(204);  // No Content
  });

  server.begin();
}

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

  // 👇 Aqui foi o ajuste crucial
  json += "],\"efficiency\":[";
  for (int i = 0; i < 10; i++) {
    json += String(efficiencyHistory[i], 2);
    if (i < 9) json += ",";
  }

  json += "],\"last_source\":" + String(tempSource, 2);
  json += ",\"last_target\":" + String(tempTarget, 2);
  json += ",\"last_cold\":" + String(tempCold, 2);
  json += ",\"max_source\":" + String(maxTempSource, 2);

  // Campos de status
  json += ",\"heater\":" + String(isRelayOn(HEATER_PIN) ? "1" : "0");
  json += ",\"cold_pump\":" + String(isRelayOn(COLD_PUMP_PIN) ? "1" : "0");
  json += ",\"target_pump\":" + String(isRelayOn(TARGET_PUMP_PIN) ? "1" : "0");
  json += ",\"mode\":" + String(manualMode ? "1" : "0");

  float coolingEfficiency = (tempTarget - tempCold) / (tempSource - tempCold);
  if (coolingEfficiency < 0) coolingEfficiency = 0;
  if (coolingEfficiency > 1) coolingEfficiency = 1;

  json += ",\"cooling_eff\":" + String(coolingEfficiency * 100.0, 1);  // em porcentagem

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
  html += "</head>";
  html += "<body style='background: var(--bg-color); color: var(--text-color);'>";

  // Header Buttons
  html += "<div class='header-buttons'>";
  html += "<button onclick=\"toggleMode()\">Toggle Mode</button>";
  html += "<button onclick=\"toggleSettings()\">Settings</button>";
  html += "<button onclick=\"toggleManual()\">Manual</button>";
  html += "<button onclick=\"toggleChart()\">Grafico</button>";
  html += "</div>";

  html += "<h1>Heat Exchanger Controller</h1>";

  html += "<div class='grid'>";

  // Manual control (ajustado com atualização imediata)
  html += "<div class='card' id='manual' style='display:none;'><h2>Manual Control</h2>";
  html += "<button id='returnAuto' style='width:100%;background:#2ecc71;color:white;' onclick=\"if (this.dataset.mode === 'manual') { fetch('/manual/off',{cache:'no-cache'}).then(() => { this.innerText = 'SWITCHING...'; this.disabled = true; updateStatus(); }); }\">AUTO MODE ACTIVE</button>";
  html += "<table>";

  // Controle do aquecedor
  html += "<tr><td>Heater</td><td>";
  html += "<button onclick=\"fetch('/manual/heater/on',{cache:'no-cache'}).then(updateStatus)\">ON</button> ";
  html += "<button onclick=\"fetch('/manual/heater/off',{cache:'no-cache'}).then(updateStatus)\">OFF</button>";
  html += "</td></tr>";

  // Controle da bomba quente
  html += "<tr><td>Hot Pump</td><td>";
  html += "<button onclick=\"fetch('/manual/target/on',{cache:'no-cache'}).then(updateStatus)\">ON</button> ";
  html += "<button onclick=\"fetch('/manual/target/off',{cache:'no-cache'}).then(updateStatus)\">OFF</button>";
  html += "</td></tr>";

  // Controle da bomba fria
  html += "<tr><td>Cold Pump</td><td>";
  html += "<button onclick=\"fetch('/manual/cold/on',{cache:'no-cache'}).then(updateStatus)\">ON</button> ";
  html += "<button onclick=\"fetch('/manual/cold/off',{cache:'no-cache'}).then(updateStatus)\">OFF</button>";
  html += "</td></tr>";

  html += "</table></div>";


  // Status
  html += "<div class='card'>";
  html += "<h2>Temperature Sensors</h2><p>";
  html += "Source Recipe: <b id='sourceTemp'>" + String(tempSource, 1) + " &deg;C</b><br>";
  html += "Target Recipe: <b id='targetTemp'>" + String(tempTarget, 1) + " &deg;C</b><br>";
  html += "Cold Recipe: <b id='coldTemp'>" + String(tempCold, 1) + " &deg;C</b><br>";
  html += "Max Temp Source: <b id='maxSourceTemp'>" + String(maxTempSource, 1) + " &deg;C</b></p>";
  html += "<tr><td>Cooling Efficiency<: /td><td><span id='cooling-eff'><b>" + String(coolingEfficiency, 1) + " %</b></span></td></tr>";

  html += "<h2>System Status</h2>";
  html += "<table style='width:100%; font-size:1em;'>";
  html += "<tr><td>Heater</td><td><span id='status-heater' class='badge " + String(isRelayOn(HEATER_PIN) ? "yes'>ON" : "no'>OFF") + "</span></td></tr>";
  html += "<tr><td>Target Pump</td><td><span id='status-target' class='badge " + String(isRelayOn(TARGET_PUMP_PIN) ? "yes'>ON" : "no'>OFF") + "</span></td></tr>";
  html += "<tr><td>Cold Pump</td><td><span id='status-cold' class='badge " + String(isRelayOn(COLD_PUMP_PIN) ? "yes'>ON" : "no'>OFF") + "</span></td></tr>";
  html += "<tr><td>Mode</td><td><span id='status-mode' class='badge " + String(manualMode ? "no'>MANUAL" : "yes'>AUTO") + "</span></td></tr>";
  html += "</table></div>";


  // Pumps Info
  html += "<div class='card'><h2>Pump Infos</h2>";

  // Diferença de temperatura
  float deltaT = tempTarget - tempCold;
  if (deltaT < 0.1) deltaT = 0.1;

  // Bombeamento
  const float pumpFlowRate = 1.0;  // L/h
  float timeFor1L = (1.0 / pumpFlowRate) * 60.0;
  String timeStr = (timeFor1L < 1.0)
                     ? String(timeFor1L * 60.0, 0) + " seg"
                     : String(timeFor1L, 1) + " min";

  // Parâmetros térmicos
  const float pumpPower = 5.0;  // W
  const float estimatedEfficiency = 0.3;
  const float targetVolume = 3.0;

  // Cálculo modular
  String coolTimeStr = calcularTempoResfriamento(tempTarget, tempCold, 1.0, pumpPower, estimatedEfficiency);
  String totalCoolTimeStr = calcularTempoResfriamento(tempTarget, tempCold, targetVolume, pumpPower, estimatedEfficiency);

  // Cold Pump
  html += "<h3 style='margin-bottom:5px;'>Cold Recipe Pump</h3>";
  html += "<p>Modo: <b>Continuo</b><br>";
  html += "Vazao: <b><span id='pumpFlow'>" + String(pumpFlowRate, 1) + "</span> L/h</b><br>";
  html += "Tempo estimado para recircular 1 L: <b id='recircTime'>" + timeStr + "</b><br>";
  html += "Tempo estimado para resfriar 1&deg;C por litro (&#x0394;T = <span id='deltaT'>" + String(deltaT, 1) + "</span>&deg;C): <b id='coolTime'>" + coolTimeStr + "</b></p>";

  // Target Pump
  html += "<h3 style='margin-bottom:5px;'>Target Recipe Pump</h3>";
  html += "<p>Mode: <b>Automatic (based on Source Recipe Temp.)</b><br>";
  html += "Vazao: <b><span id='pumpFlow2'>" + String(pumpFlowRate, 1) + "</span> L/h</b><br>";
  html += "Tempo estimado para recircular 1 L: <b id='recircTime2'>" + timeStr + "</b><br>";
  html += "Tempo estimado para resfriar " + String(targetVolume, 1) + " L em 1&deg;C (&#x0394;T = <span id='deltaT2'>" + String(deltaT, 1) + "</span>&deg;C): <b id='totalCoolTime'>" + totalCoolTimeStr + "</b></p>";

  html += "</div>";


  // Settings
  html += "<div class='card' id='settings' style='display:none;'><h2>Settings</h2>";
  html += "<p>No configurable flow parameters. Pumps run continuously or automatically based on system logic.</p>";
  html += "<button onclick=\"fetch('/reset_history').then(()=>alert('History and Max Temp cleared!'));\">Reset History</button>";
  html += "</div>";


  // Chart Box
  html += "<div class='card' id='chartBox' style='grid-column: span 4; display: none;'>";
  html += "<h2>Temperature Graph</h2>";
  html += "<canvas id='chart' style='max-height: 300px; width: 100%;'></canvas>";
  html += "</div>";

  // JS to toggle, charts and updates
  html += "<script>";
  html += "function toggleMode() { document.body.classList.toggle('light'); }";
  html += "function toggleSettings() { var s = document.getElementById('settings'); s.style.display = (s.style.display === 'none') ? 'block' : 'none'; }";
  html += "function toggleManual() { var s = document.getElementById('manual'); s.style.display = (s.style.display === 'none') ? 'block' : 'none'; }";

  html += "function toggleChart() {";
  html += "  var s = document.getElementById('chartBox');";
  html += "  if (s.style.display === 'none') {";
  html += "    if (!document.getElementById('chartjs-script')) {";
  html += "      const scr = document.createElement('script');";
  html += "      scr.src = '/chart.min.js?v=' + Date.now();";
  html += "      scr.id = 'chartjs-script';";
  html += "      scr.onload = () => { console.log('✅ Chart.js loaded'); renderChart(); };";
  html += "      scr.onerror = () => console.error('❌ Failed to load Chart.js');";
  html += "      document.head.appendChild(scr);";
  html += "    } else {";
  html += "      renderChart();";
  html += "    }";
  html += "  } else { s.style.display = 'none'; }";
  html += "}";

  // Chart dynamic load + render
  html += "function renderChart() {";
  html += "  console.log('renderChart iniciado');";
  html += "  const chartBox = document.getElementById('chartBox');";
  html += "  chartBox.style.display = 'block';";
  html += "  const canvas = document.getElementById('chart');";
  html += "  const ctx = canvas.getContext('2d');";
  html += "  window.chart = new Chart(ctx, {";
  html += "    type: 'line',";
  html += "    data: {";
  html += "      labels: [],";
  html += "      datasets: [";
  html += "        { label: 'Source Recipe', data: [], borderColor: 'red', fill: false, tension: 0.4 },";
  html += "        { label: 'Target Recipe', data: [], borderColor: 'orange', fill: false, tension: 0.4 },";
  html += "        { label: 'Cold Recipe', data: [], borderColor: 'blue', fill: false, tension: 0.4 }";
  html += "      ]";
  html += "    },";
  html += "    options: {";
  html += "      animation: " + String(animationEnabled ? "{}" : "{ duration: 0 }") + ",";
  html += "      maintainAspectRatio: false,";
  html += "      scales: {";
  html += "        x: { title: { display: true, text: 'Time (s) since boot' } },";
  html += "        y: { title: { display: true, text: 'Temperature (\\u00B0C)' }, position: 'left' }";
  html += "      }";
  html += "    }";
  html += "  });";
  html += "}";


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

  // Update Status para os botoes
  html += "function updateStatus() {";
  html += "  fetch('/data').then(r => r.json()).then(d => {";
  html += "    document.getElementById('status-heater').className = 'badge ' + (d.heater ? 'yes' : 'no');";
  html += "    document.getElementById('status-heater').innerText = d.heater ? 'ON' : 'OFF';";

  html += "    document.getElementById('status-cold').className = 'badge ' + (d.cold_pump ? 'yes' : 'no');";
  html += "    document.getElementById('status-cold').innerText = d.cold_pump ? 'ON' : 'OFF';";

  html += "    document.getElementById('status-target').className = 'badge ' + (d.target_pump ? 'yes' : 'no');";
  html += "    document.getElementById('status-target').innerText = d.target_pump ? 'ON' : 'OFF';";

  html += "    document.getElementById('status-mode').className = 'badge ' + (d.mode ? 'no' : 'yes');";
  html += "    document.getElementById('status-mode').innerText = d.mode ? 'MANUAL' : 'AUTO';";

  html += "    const returnBtn = document.getElementById('returnAuto');";
  html += "    if (returnBtn) {";
  html += "      returnBtn.disabled = !d.mode;";
  html += "      returnBtn.dataset.mode = d.mode ? 'manual' : 'auto';";
  html += "      returnBtn.innerText = d.mode ? 'RETURN TO AUTO MODE' : 'AUTO MODE ACTIVE';";
  html += "    }";
  html += "  });";
  html += "}";


  // Automatic Updates
  html += "setInterval(() => {";
  html += "  fetch('/data').then(r => r.json()).then(d => {";

  // 🔄 Atualização do gráfico (se carregado)
  //html += "    if (window.chart) {";
  html += "    if (window.chart && chart.data && chart.data.labels) {";
  html += "      chart.data.labels = [...Array(historySize).keys()].map(i => i * historyInterval);";
  html += "      chart.data.datasets[0].data = d.source;";
  html += "      chart.data.datasets[1].data = d.target;";
  html += "      chart.data.datasets[2].data = d.cold;";
  html += "      chart.update();";
  html += "    }";

  // 🔄 Atualização das temperaturas no painel
  html += "    document.getElementById('sourceTemp').innerHTML = d.last_source.toFixed(1) + ' \\u00B0C';";
  html += "    document.getElementById('targetTemp').innerHTML = d.last_target.toFixed(1) + ' \\u00B0C';";
  html += "    document.getElementById('coldTemp').innerHTML = d.last_cold.toFixed(1) + ' \\u00B0C';";

  // 🔄 Atualização dos tempos estimados de resfriamento
  html += "    let deltaT = d.last_target - d.last_cold;";
  html += "    if (deltaT < 0.1) deltaT = 0.1;";
  html += "    const coolingEnergy = 4186.0;";
  html += "    const pumpPower = 5.0;";
  html += "    const efficiency = 0.3;";
  html += "    const effectivePower = pumpPower * efficiency;";
  html += "    const baseTimePerL = coolingEnergy / effectivePower;";
  html += "    const adjustedPerL = baseTimePerL / deltaT;";
  html += "    const adjustedTotal = adjustedPerL * 3.0;";

  html += "    document.getElementById('deltaT').innerText = deltaT.toFixed(1);";
  html += "    document.getElementById('coolTime').innerText = (adjustedPerL < 60 ? Math.round(adjustedPerL) + ' seg' : (adjustedPerL / 60).toFixed(1) + ' min');";
  html += "    document.getElementById('totalCoolTime').innerText = (adjustedTotal < 60 ? Math.round(adjustedTotal) + ' seg' : (adjustedTotal / 60).toFixed(1) + ' min');";

  // 🔄 Atualização da temperatura máxima do Source
  html += "    if (document.getElementById('maxSourceTemp')) {";
  html += "      document.getElementById('maxSourceTemp').innerHTML = d.max_source.toFixed(1) + ' \\u00B0C';";
  html += "    }";

  // 🔄 Atualização da eficiência de resfriamento no painel
  html += "    document.getElementById('cooling-eff').innerHTML = '<b>' + d.cooling_eff.toFixed(1) + ' %</b>';";

  // 🔄 Atualização dos status de relés e modo
  html += "    document.getElementById('status-heater').className = 'badge ' + (d.heater ? 'yes' : 'no');";
  html += "    document.getElementById('status-heater').innerText = d.heater ? 'ON' : 'OFF';";

  html += "    document.getElementById('status-cold').className = 'badge ' + (d.cold_pump ? 'yes' : 'no');";
  html += "    document.getElementById('status-cold').innerText = d.cold_pump ? 'ON' : 'OFF';";

  html += "    document.getElementById('status-target').className = 'badge ' + (d.target_pump ? 'yes' : 'no');";
  html += "    document.getElementById('status-target').innerText = d.target_pump ? 'ON' : 'OFF';";

  html += "    if (document.getElementById('status-valve')) {";
  html += "      document.getElementById('status-valve').className = 'badge ' + (d.valve ? 'yes' : 'no');";
  html += "      document.getElementById('status-valve').innerText = d.valve ? 'OPEN' : 'CLOSED';";
  html += "    }";

  html += "    document.getElementById('status-mode').className = 'badge ' + (d.mode ? 'no' : 'yes');";
  html += "    document.getElementById('status-mode').innerText = d.mode ? 'MANUAL' : 'AUTO';";

  // 🔄 Atualização do botão de retorno automático
  html += "    if (document.getElementById('returnAuto')) {";
  html += "      document.getElementById('returnAuto').disabled = !d.mode;";
  html += "      document.getElementById('returnAuto').innerText = 'RETURN TO AUTO MODE';";
  html += "    }";

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