# Smart Irrigation System (IoT Based) 🌿

An automated IoT irrigation system designed to optimize water usage by monitoring environmental conditions in real-time using ESP32 and Firebase.

## 🚀 Features
- **Real-time Monitoring:** Tracks soil moisture, temperature, and humidity.
- **Automated Control:** The pump activates automatically based on sensor thresholds.
- **Safety Trip Logic:** Includes a "Cooldown" and "Safety Trip" mechanism to protect the pump from dry running or overheating.
- **Cloud Integration:** Firebase Realtime Database for data logging and remote monitoring.
- **Android App:** Dedicated mobile application for control and status updates.

## 🛠 Hardware Components
- **Microcontroller:** ESP32 (NodeMCU).
- **Sensors:** DHT11 (Temp/Hum), Soil Moisture Sensor, LDR (Light).
- **Actuators:** 5V Relay Module, Water Pump.
- **Others:** LCD 16x2 (I2C), Power Supply.

## 💻 Tech Stack
- **Firmware:** C++ (Arduino Framework).
- **Cloud:** Firebase Realtime Database.
- **Documentation:** Proteus for circuit simulation.

## 📂 Project Structure
- `smart_irrigation.ino`: The main firmware source code.
- `app-arm64-v8a-release.apk`: The mobile application for the system.
- `proteus.pdf`: Circuit schematic and simulation layout.
- `H.W.jpg`: Actual hardware implementation photo.

## 🔧 Safety Logic (Code Snippet)
The system implements a protection layer for the pump:
- If the pump runs for more than X minutes, it enters **Cooldown Mode**.
- Emergency stop if no water flow is detected (Safety Trip).

---
*Developed as part of my Electrical Engineering projects.*
