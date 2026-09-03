// ─── History Buffers ─────────────────────────────────────────────────────────
let rmsHistory       = [];
let overloadHistory  = [];
let transientHistory = [];
let averageRMS       = 0;

// ─── Combined Chart: RMS + Amp + Transient ───────────────────────────────────
const ctxVib = document.getElementById('vibrationChart').getContext('2d');
const vibChart = new Chart(ctxVib, {
  type: 'line',
  data: {
    labels: [],
    datasets: [
      {
        label: 'Vibration RMS (g)',
        data: [],
        borderColor: '#1e88e5',
        backgroundColor: 'rgba(30,136,229,0.08)',
        borderWidth: 2.5,
        tension: 0.3,
        pointRadius: 0,
        fill: true,
        yAxisID: 'yRms'
      },
      {
        label: 'Operation Current (A)',
        data: [],
        borderColor: '#e53935',
        backgroundColor: 'rgba(229,57,53,0.06)',
        borderWidth: 2,
        tension: 0.3,
        pointRadius: 0,
        fill: false,
        yAxisID: 'yAmp'
      },
      {
        label: 'Transient Voltage (V)',
        data: [],
        borderColor: '#8e24aa',
        backgroundColor: 'rgba(142,36,170,0.06)',
        borderWidth: 2,
        tension: 0.3,
        pointRadius: 0,
        fill: false,
        yAxisID: 'yTransient'
      }
    ]
  },
  options: {
    responsive: true,
    animation: false,
    interaction: { mode: 'index', intersect: false },
    plugins: {
      legend: { position: 'top' },
      title: {
        display: true,
        text: 'Live Motor Data — RMS | Current | Transient Voltage',
        font: { size: 15 }
      }
    },
    scales: {
      yRms: {
        type: 'linear',
        display: true,
        position: 'left',
        title: { display: true, text: 'Vibration RMS (g)', color: '#1e88e5' },
        ticks: { color: '#1e88e5' },
        beginAtZero: true,
        suggestedMax: 0.1,
        grid: { drawOnChartArea: true }
      },
      yAmp: {
        type: 'linear',
        display: true,
        position: 'right',
        title: { display: true, text: 'Current (A)', color: '#e53935' },
        ticks: { color: '#e53935' },
        beginAtZero: true,
        suggestedMax: 10,
        grid: { drawOnChartArea: false }
      },
      yTransient: {
        type: 'linear',
        display: true,
        position: 'right',
        title: { display: true, text: 'Voltage (V)', color: '#8e24aa' },
        ticks: { color: '#8e24aa' },
        beginAtZero: true,
        suggestedMax: 20,
        grid: { drawOnChartArea: false },
        offset: true
      },
      x: { title: { display: true, text: 'Time' } }
    }
  }
});

// ─── Add point to chart ───────────────────────────────────────────────────────
function addToVibChart(rms, amp, transient) {
  const ts = new Date().toLocaleTimeString();
  vibChart.data.labels.push(ts);
  vibChart.data.datasets[0].data.push(rms);
  vibChart.data.datasets[1].data.push(amp);
  vibChart.data.datasets[2].data.push(transient);

  if (vibChart.data.labels.length > 60) {
    vibChart.data.labels.shift();
    vibChart.data.datasets.forEach(ds => ds.data.shift());
  }
  vibChart.update();
}

// ─── Card Updaters ────────────────────────────────────────────────────────────
function updateDashboard(rms) {
  document.getElementById("rmsValue").innerText = rms.toFixed(4) + " g";
}

function updateAverageRMS(rms) {
  rmsHistory.push(rms);
  if (rmsHistory.length > 60) rmsHistory.shift();

  const sum = rmsHistory.reduce((a, b) => a + b, 0);
  averageRMS = sum / rmsHistory.length;

  const el = document.getElementById("avgValue");
  el.innerText = averageRMS.toFixed(4) + " g";
  if (averageRMS < 0.3)      el.style.color = "green";
  else if (averageRMS < 0.6) el.style.color = "orange";
  else                        el.style.color = "red";

  document.getElementById("sbAvgVibration").innerText = averageRMS.toFixed(4) + " g";
  updateMotorCondition(averageRMS);
}

function updateOverload(current) {
  const el = document.getElementById("overloadValue");
  el.innerText = current.toFixed(2) + " A";
  el.style.color = current > 10 ? "red" : "#1e88e5";
}

function updateAverageOverload(current) {
  overloadHistory.push(current);
  if (overloadHistory.length > 7) overloadHistory.shift();

  const sum = overloadHistory.reduce((a, b) => a + b, 0);
  const avgLoad = sum / overloadHistory.length;

  const el = document.getElementById("avgOverload");
  el.innerText = avgLoad.toFixed(2) + " A";
  if (avgLoad <= 10)      el.style.color = "green";
  else if (avgLoad <= 20) el.style.color = "orange";
  else                     el.style.color = "red";

  document.getElementById("sbAvgOverload").innerText = avgLoad.toFixed(2) + " A";
  updateRemainingLife(averageRMS, avgLoad);
}

function updateTransientVoltage(voltage) {
  const el = document.getElementById("transientValue");
  el.innerText = voltage.toFixed(2) + " V";
  el.style.color = voltage > 250 ? "red" : "#1e88e5";
}

function updateAverageTransient(voltage) {
  transientHistory.push(voltage);
  if (transientHistory.length > 60) transientHistory.shift();

  const sum = transientHistory.reduce((a, b) => a + b, 0);
  const avg = sum / transientHistory.length;

  const el = document.getElementById("avgTransient");
  el.innerText = avg.toFixed(2) + " V";
  if (avg < 220)       el.style.color = "orange";
  else if (avg <= 250) el.style.color = "green";
  else                  el.style.color = "red";

  document.getElementById("sbAvgTransient").innerText = avg.toFixed(2) + " V";
}

// ─── Fetch from ESP32 every 1 s ───────────────────────────────────────────────
setInterval(() => {
  fetch("/data")
    .then(response => response.json())
    .then(data => {
      const rms      = data.rms      ?? 0;
      const amp      = data.amp      ?? 0;
      const transient = data.transient ?? 0;

      updateDashboard(rms);
      updateAverageRMS(rms);
      addToVibChart(rms, amp, transient);

      updateOverload(amp);
      updateAverageOverload(amp);

      updateTransientVoltage(transient);
      updateAverageTransient(transient);
    })
    .catch(err => console.log("ESP32 not reachable:", err));
}, 1000);

// ─── Relay toggle ─────────────────────────────────────────────────────────────
const relayToggle = document.getElementById("relayToggle");
const relayStatus = document.getElementById("relayStatus");

relayToggle.addEventListener("change", () => {
  const state = relayToggle.checked ? 1 : 0;
  relayStatus.innerText = state ? "ON" : "OFF";
  fetch(`/relay?state=${state}`)
    .then(r => r.text())
    .then(t => console.log("Relay response:", t))
    .catch(err => console.log("ESP32 not reachable:", err));
});

function getRelayStateFromESP() {
  fetch("/data")
    .then(r => r.json())
    .then(data => {
      relayToggle.checked = (data.relay === "ON");
      relayStatus.innerText = data.relay;
    })
    .catch(err => console.log("Relay JSON error:", err));
}
setInterval(getRelayStateFromESP, 2000);
getRelayStateFromESP();

// ─── Sidebar ──────────────────────────────────────────────────────────────────
const sidebar = document.getElementById("sidebar");
const menuBtn = document.getElementById("menuBtn");
const overlay = document.getElementById("overlay");

menuBtn.addEventListener("click", () => {
  sidebar.classList.toggle("active");
  overlay.classList.toggle("active");
});
overlay.addEventListener("click", () => {
  sidebar.classList.remove("active");
  overlay.classList.remove("active");
});

// ─── Motor condition ──────────────────────────────────────────────────────────
function updateMotorCondition(avgVibration) {
  const statusEl = document.getElementById("systemStatus");
  if (avgVibration <= 0.05) {
    statusEl.innerText = "Normal";           statusEl.style.color = "green";
  } else if (avgVibration <= 0.15) {
    statusEl.innerText = "Stable";           statusEl.style.color = "#f9a825";
  } else if (avgVibration <= 0.30) {
    statusEl.innerText = "Warning";          statusEl.style.color = "orange";
  } else if (avgVibration <= 0.60) {
    statusEl.innerText = "Critical";         statusEl.style.color = "red";
  } else {
    statusEl.innerText = "Shutdown Required"; statusEl.style.color = "darkred";
  }
}

// ─── Remaining life ───────────────────────────────────────────────────────────
function updateRemainingLife(avgVibration, avgCurrent) {
  const lifeEl = document.getElementById("remainingHours");
  const RATED_CURRENT   = 20;
  const BASE_LIFE_HOURS = 100;

  if (avgVibration <= 0.60) {
    lifeEl.innerText   = "Safe";
    lifeEl.style.color = "green";
    return;
  }
  const vibFactor    = avgVibration / 0.60;
  const curFactor    = avgCurrent   / RATED_CURRENT;
  const stressFactor = vibFactor * curFactor;
  let remaining      = BASE_LIFE_HOURS / stressFactor;
  remaining          = Math.max(1, Math.min(remaining, BASE_LIFE_HOURS));

  lifeEl.innerText   = remaining.toFixed(1);
  lifeEl.style.color = "red";
}
