# THOS v2.1 – M5Stack Dial Firmware

<!-- OSS Badges -->

![License](https://img.shields.io/github/license/TelemetryHarbor/thos-m5stack-dial.svg)
![Last Commit](https://img.shields.io/github/last-commit/TelemetryHarbor/thos-m5stack-dial.svg)
![Issues](https://img.shields.io/github/issues/TelemetryHarbor/thos-m5stack-dial.svg)
![Pull Requests](https://img.shields.io/github/issues-pr/TelemetryHarbor/thos-m5stack-dial.svg)
![Repo Size](https://img.shields.io/github/repo-size/TelemetryHarbor/thos-m5stack-dial.svg)
![Contributors](https://img.shields.io/github/contributors/TelemetryHarbor/thos-m5stack-dial.svg)
![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)
![Stars](https://img.shields.io/github/stars/TelemetryHarbor/thos-m5stack-dial.svg?style=social)
![Forks](https://img.shields.io/github/forks/TelemetryHarbor/thos-m5stack-dial.svg?style=social)

THOS v2.1 is a fully-featured open-source firmware for the **M5Stack Dial**.
It brings **watch, alarm, timer, and settings apps** in a lightweight, smooth, and responsive UI with **OLED graphics**, rotary encoder input, and tactile feedback.



## 🔥 Features

* ⌚ **Smart Watch** – Highly optimized, low-power clock with 12/24h toggle
* ⏱️ **Timer** – Set, start, pause, and ring timers with gauge progress
* ⏰ **Alarm** – Polling-based alarm with snooze and dynamic ringing alerts
* ⚙️ **Settings** – Wi-Fi NTP sync, 12/24h mode toggle, brightness control
* 🎨 **Industrial Palette UI** – Modern, minimal, and vibrant design
* 📈 **Smart Redraw & Efficient Background Tasks** – Minimal CPU usage
* 🛠️ **Extensible App System** – Easy to add new apps via the `App` interface
* 🎵 **Audio Alerts** – Tone support via M5Dial speaker
* 🌙 **AOD Mode** – Screensaver with low-brightness display
* 💡 **Rotary & Touch Input Support** – Smooth, debounced interaction



## 🚀 Quick Start

1. Clone the repository:

```bash
git clone https://github.com/YourUsername/THOS-v2.1.git
cd THOS-v2.1
```

2. Open in **Arduino IDE** or **PlatformIO**.
3. Install required libraries:

   * `M5Dial`
   * `M5GFX`
4. Configure your Wi-Fi and timezone in `main.cpp`:

```cpp
const char* WIFI_SSID = "YOUR_SSID";     
const char* WIFI_PASS = "YOUR_PASSWORD"; 
#define TIME_ZONE_OFFSET_HRS +3
```

5. Build and upload to your **M5Stack Dial**.



## 📋 Supported Hardware

* **M5Stack Dial**

  * M5Core ESP32
  * Built-in OLED 240×240
  * Rotary encoder + button
  * Speaker for alarms



## ⚡ UI & Interaction

* **Menu Navigation**

  * Rotate encoder → Switch apps
  * Tap screen → Select / confirm
  * BtnA → Back / exit app

* **Watch App**

  * Smart redraw to save power
  * Shows date, time, and AM/PM if 12h mode

* **Timer App**

  * Rotary = adjust time (coarse/fine)
  * Tap = start/pause/stop
  * Ringing = dynamic gauge + tone

* **Alarm App**

  * Rotary = toggle ON/OFF or adjust hour/min
  * Tap = snooze when ringing
  * Background polling every 500ms for accurate alarm trigger

* **Settings App**

  * Tap top half = NTP sync over Wi-Fi
  * Tap bottom half = toggle 12/24h format
  * Feedback tones for success/failure



## 🔧 Configuration Options

| Parameter              | Description                        | Default |
| ---------------------- | ---------------------------------- | ------- |
| `TIME_ZONE_OFFSET_HRS` | Offset from UTC                    | +3      |
| `BRIGHT_ACTIVE`        | Active display brightness (0–255)  | 200     |
| `BRIGHT_AOD`           | Screensaver/AOD brightness (0–255) | 10      |
| `SCREENSAVER_TIMEOUT`  | Inactivity before AOD (ms)         | 20000   |
| `C_ACCENT`             | Main accent color                  | 0xFB20  |



## 🛠️ Extending THOS

THOS uses an `App` interface for modularity.
Add your own app:

```cpp
class MyApp : public App {
    void setup() override {}
    void loop() override {}
    void draw() override {}
    void onRotary(int delta) override {}
    void onTouch(int x, int y) override {}
    String getName() override { return "MY APP"; }
};
```

Then register in `setup()`:

```cpp
apps.push_back(new MyApp());
```



## ⚠️ Notes

* Wi-Fi NTP sync may take a few seconds; firmware handles retry automatically.
* Timer & Alarm override menu for priority alerts.
* Rotary encoder sensitivity can be adjusted by changing `abs(rawDelta) >= 4` threshold.



## 📜 License

This project is licensed under **Apache License 2.0**.

* ✅ Free for personal and commercial use
* ✅ Modify and redistribute allowed
* ⚠️ Must include attribution



## 🤝 Contributing

We welcome issues, pull requests, and feature ideas!

* Open a GitHub issue for bugs or enhancements
* Fork the repo and submit PRs
* Join discussions and improve THOS together
