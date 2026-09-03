# Industrial Equipment Monitoring System

An embedded industrial monitoring prototype built around the **ESP32** that collects real-time vibration data from an **MPU6050 accelerometer/gyroscope** and provides a web-based interface for monitoring and visualizing the collected data.

The ESP32 acts as both the **data acquisition controller and web server**, allowing vibration information to be viewed directly through a browser without requiring a separate server or cloud platform.

---

## 📌 Project Overview

Vibration is an important indicator of the operating condition of industrial equipment. Changes in vibration patterns can provide useful information about abnormal operation, mechanical imbalance, looseness, misalignment, or other potential issues.

This prototype explores a simple approach to **embedded vibration monitoring** by combining:

- ESP32 for processing and networking
- MPU6050 for vibration/motion sensing
- A web server hosted directly on the ESP32
- Real-time data visualization through a web interface

The system continuously acquires vibration data from the MPU6050 and makes the data available through a locally hosted website, where it can be plotted and monitored.

---

## ⚙️ System Architecture

```text
                 ┌──────────────────────┐
                 │       MPU6050        │
                 │ Accelerometer + Gyro  │
                 └──────────┬───────────┘
                            │
                            │ I²C
                            ▼
                 ┌──────────────────────┐
                 │        ESP32         │
                 │                      │
                 │  Sensor Acquisition  │
                 │       +              │
                 │   Data Processing    │
                 │       +              │
                 │    Web Server        │
                 └──────────┬───────────┘
                            │
                            │ Wi-Fi
                            ▼
                 ┌──────────────────────┐
                 │     Web Browser      │
                 │                      │
                 │  Vibration Data      │
                 │      Plot            │
                 └──────────────────────┘
