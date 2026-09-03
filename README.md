# Industrial Equipment Monitoring System

An embedded industrial monitoring prototype built around the **ESP32** that collects vibration data from an **MPU6050 accelerometer/gyroscope** and provides a web-based interface for monitoring, visualizing, and evaluating the condition of a motor.

The ESP32 acts as both the **data acquisition controller and web server**, allowing vibration information to be viewed directly through a browser without requiring a separate server or cloud platform.

---

## 📌 Project Overview

Vibration is an important indicator of the operating condition of rotating industrial equipment.

A healthy motor generally produces a relatively consistent vibration pattern during steady-state operation. Changes in the vibration level or pattern can indicate potential mechanical problems such as:

- Mechanical imbalance
- Misalignment
- Bearing degradation
- Looseness
- Unusual mechanical loading
- Abnormal operating conditions

This prototype explores a simple approach to **motor condition monitoring** by acquiring vibration data and visualizing it through a web interface hosted directly on the ESP32.

The system combines:

- **ESP32** for data acquisition, processing, and networking
- **MPU6050** for vibration/motion sensing
- **I²C** for sensor communication
- **Wi-Fi** for communication with the monitoring interface
- **Web server hosted on ESP32** for data visualization
- **Vibration plot** for observing motor behavior

---

## ⚙️ System Architecture

```text
                 ┌──────────────────────┐
                 │       MOTOR          │
                 │   Rotating Machine   │
                 └──────────┬───────────┘
                            │
                       Mechanical
                        Vibration
                            │
                            ▼
                 ┌──────────────────────┐
                 │       MPU6050        │
                 │                      │
                 │  3-Axis Accelerometer│
                 │  3-Axis Gyroscope    │
                 └──────────┬───────────┘
                            │
                            │ I²C
                            ▼
                 ┌──────────────────────┐
                 │        ESP32         │
                 │                      │
                 │ Sensor Acquisition   │
                 │        +             │
                 │ Data Processing      │
                 │        +             │
                 │ Condition Analysis   │
                 │        +             │
                 │ Web Server            │
                 └──────────┬───────────┘
                            │
                            │ Wi-Fi
                            ▼
                 ┌──────────────────────┐
                 │     Web Browser      │
                 │                      │
                 │ Vibration Data       │
                 │        +             │
                 │ Vibration Plot       │
                 │        +             │
                 │ Motor Condition      │
                 └──────────────────────┘
## 🔄 Motor Condition Monitoring Workflow
Raw Vibration Data
        │
        ▼
   Data Acquisition
        │
        ▼
 Signal Processing
        │
        ▼
 Vibration Pattern
    Observation
        │
        ▼
 Condition Evaluation
        │
        ├───────────────┐
        ▼               ▼
     Normal          Abnormal
        │               │
        ▼               ▼
 Continue          Inspection /
 Monitoring        Maintenance
