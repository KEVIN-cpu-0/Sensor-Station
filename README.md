# Ag Sensor Station Challenge 📡🌾

Welcome to the **Ag Sensor Station Problem**. This challenge focuses on precision agriculture, microcontrollers, and long-range communication in environments where the internet isn't guaranteed.

## 2026 IoT4Ag Hackathon - Team 2 - Sensor Control
Team Members:
* [@DoppelgangerVortex](https://github.com/DoppelgangerVortex)
* [@KEVIN-cpu-0](https://github.com/KEVIN-cpu-0)
* [@Ydde7](https://github.com/Ydde7)
* [@EZ-Pixel-Dev](https://github.com/EZ-Pixel-Dev)

## 🚀 The Challenge
In modern farming, data is king, but connectivity is often missing. We are tasked with connecting two stations via **LoRa (Long Range)** WAN.

### Core Objectives
* **Serialization:** Pack sensor data efficiently for radio transmission.
* **LoRa Communication:** Bridge the gap between two stations without Wi-Fi/Cellular.
* **Sim-to-Real:** Develop your logic in a **Docker** simulation and (optionally) deploy it to real **Arduino** hardware.

## Real Life Scenario
Winemakers and brewers face a major challenge during fermentation: small changes in conditions like temperature, CO2, humidity, pressure, and pH can quickly ruin an entire batch. If the temperature gets too high, yeast can die. If it gets too low, fermentation can slow or stop. Poor CO2 balance can negatively affect flavor, stability, and quality, while shifts in pH and moisture can further disrupt the process. The problem is that these changes are not always caught early enough.

## Our Solution - Fermentation Monitoring System
We’re developing a smart fermentation monitoring system that uses sensors to track key conditions like temperature, pH, and gas activity during alcohol production, then sends that data to a simple interface for real-time viewing. Instead of relying only on manual checks, brewers or producers can monitor the fermentation process more accurately, catch problems earlier, and make better decisions to improve consistency and quality.

## Some Added Features
* The user can choose what type of alcohol they wish to brew. Current selections are Ale, Lager, Red Wine, and White Wine. If pressure, humidity, CO2, pH, and temperature conditions reach a certain limit, the program alerts the owner.
...We had more planned, but that's it for now.

## 🛠️ Tech Stack
* **Hardware:** Arduino / ESP32 + LoRa Modules
* **Simulation:** Docker
* **Communication:** Serial, LoRaWAN
* **Languages:** C++/Arduino

## Presentation
* Canva Presentation: https://canva.link/ufqpi3q7dw2j6nu
* Google Doc: https://docs.google.com/document/d/1I-2Qh9vmKXN3N25oVqZpfofVYi6o4RvfuPTzUwevTeE/edit?usp=sharing

## 📦 Getting Started
1.  **Clone the repo:**
    ```bash
    git clone https://github.com/your-repo/ag-sensor-station.git
    ```
2.  **Hardware Testing:**
    Once your code works in the sim, flash it to the physical Arduino stations to see the sensors in action.
---
*Part of the Precision Ag Embedded Systems Series.*
