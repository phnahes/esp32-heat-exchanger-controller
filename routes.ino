// Routes

void handleData() {
  String json = "{";

  json += "\"source\":[";
  for (int i = 0; i < 10; i++) {
    int index = (currentIndex + i) % 10;
    json += String(sourceTempHistory[index]);
    if (i < 9) json += ",";
  }

  json += "],\"target\":[";
  for (int i = 0; i < 10; i++) {
    int index = (currentIndex + i) % 10;
    json += String(targetTempHistory[index]);
    if (i < 9) json += ",";
  }

  json += "],\"cold\":[";
  for (int i = 0; i < 10; i++) {
    int index = (currentIndex + i) % 10;
    json += String(coldTempHistory[index]);
    if (i < 9) json += ",";
  }

  // Novo: também enviar última leitura atual para atualizar o box de sensores
  int lastIndex = (currentIndex + 9) % 10;
  json += "],\"last_source\":" + String(sourceTempHistory[lastIndex]);
  json += ",\"last_target\":" + String(targetTempHistory[lastIndex]);
  json += ",\"last_cold\":" + String(coldTempHistory[lastIndex]);

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
  html += "@media (max-width: 768px) {";
  html += "  .grid { max-width: 100%; }";
  html += "  .card {grid-column: span 1 !important;}";
  html += "}";
  html += "</style>";
  html += "<script src=\"/chart.min.js\"></script></head>";
  html += "<body style='background: var(--bg-color); color: var(--text-color);'>";
  html += "<div style='text-align:right;'><button onclick=\"toggleMode()\">Toggle Light/Dark</button> ";
  html += "<button onclick=\"toggleSettings()\">Show/Hide Settings</button></div>";
  html += "<h1>Heat Exchanger Controller</h1>";

  html += "<div class='grid'>";

  html += "<div class='card' id='settings' style='display:none;'><h2>Adjust Settings</h2><form action='/set'>";
  html += "<label>Cold Interval (ms):</label><input name='interval' value='" + String(coldPumpInterval) + "'><br>";
  html += "<label>Cold Duration (ms):</label><input name='duration' value='" + String(coldPumpDuration) + "'><br>";
  html += "<label>Target Interval (ms):</label><input name='target_interval' value='" + String(targetPumpInterval) + "'><br>";
  html += "<label>Target Duration (ms):</label><input name='target_duration' value='" + String(targetPumpDuration) + "'><br>";
  html += "<br><button type='submit'>Save</button></form>";
  html += "<button onclick=\"fetch('/reset_history').then(()=>alert('History cleared!'));\">Reset History</button></div>";

  html += "<div class='card'><h2>Sensors</h2><p>";
  html += "Source Recipe Temp.: <b>" + String(tempSource, 1) + " &deg;C</b><br>";
  html += "Target Recipe Temp.: <b>" + String(tempTarget, 1) + " &deg;C</b><br>";
  html += "Cold Recipe Temp.: <b>" + String(tempCold, 1) + " &deg;C</b></p></div>";

  html += "<div class='card'><h2>Water Level</h2>";
  html += "<p>Status: <span class='badge " + String(levelDetected ? "yes'>YES" : "no'>NO") + "</span></p></div>";

  html += "<div class='card'><h2>Cold Pump</h2>";
  html += "<p>Interval: <b>" + String(coldPumpInterval) + " ms</b><br>";
  html += "Duration: <b>" + String(coldPumpDuration) + " ms</b><br>";
  html += "Duty: <b>" + String(coldDuty, 1) + "%</b><br>";
  html += "Flow: <b>" + String(estimatedColdFlow, 1) + " L/h</b></p></div>";

  html += "<div class='card'><h2>Target Pump</h2>";
  html += "<p>Interval: <b>" + String(targetPumpInterval) + " ms</b><br>";
  html += "Duration: <b>" + String(targetPumpDuration) + " ms</b><br>";
  html += "Duty: <b>" + String(targetDuty, 1) + "%</b><br>";
  html += "Flow: <b>" + String(estimatedTargetFlow, 1) + " L/h</b></p></div>";

  html += "<div class='card' style='grid-column: span 2;'><h2>Temperature Graph</h2>";
  html += "<canvas id='chart'></canvas></div>";

  html += "</div>";

  html += "<script>";
  html += "function toggleMode(){document.body.classList.toggle('light');}";
  html += "function toggleSettings(){var s=document.getElementById('settings'); s.style.display=(s.style.display==='none')?'block':'none';}";
  html += "let ctx=document.getElementById('chart').getContext('2d');";
  html += "let chart=new Chart(ctx,{type:'line',data:{labels:[],datasets:[{label:'Source',data:[],borderColor:'red',fill:false},{label:'Target',data:[],borderColor:'blue',fill:false},{label:'Cold',data:[],borderColor:'green',fill:false}]},options:{scales:{x:{title:{display:true,text:'Time (s)'}},y:{title:{display:true,text:'Temperature (\u00B0C)'}}}}});";

  html += "fetch('/data').then(r=>r.json()).then(d=>{";
  html += "chart.data.labels = [...Array(10).keys()];";
  html += "chart.data.datasets[0].data = d.source;";
  html += "chart.data.datasets[1].data = d.target;";
  html += "chart.data.datasets[2].data = d.cold;";
  html += "chart.update();});";
  html += "setInterval(()=>{fetch('/data').then(r=>r.json()).then(d=>{";
  html += "chart.data.labels = [...Array(10).keys()];";
  html += "chart.data.datasets[0].data = d.source;";
  html += "chart.data.datasets[1].data = d.target;";
  html += "chart.data.datasets[2].data = d.cold;";
  html += "chart.update();});},5000);";

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