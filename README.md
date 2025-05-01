# 🤖 ML-Powered RC Car with ESP32-S3

An embedded AI project where a custom-trained machine learning model controls an RC car in real time using sensor inputs — no human driving required.

Powered by **ESP32-S3**, this project combines lightweight model inference and real-time motor control using onboard Time-of-Flight sensors. The goal is to enable the RC car to navigate tracks autonomously by reacting to its surroundings.

---

## 🔍 What It Does

- Collects distance measurements from sensors mounted on the RC car.
- Runs a lightweight neural network model on-device to **predict steering and speed changes**.
- Adjusts motor control dynamically, allowing the car to avoid obstacles and follow curved tracks.

The system is designed to be fully **edge-deployed** — no external server, no internet — just a smart chip inside a small car making all the decisions.

---

## 🧠 How It Works

- The model was trained on custom driving data to predict **delta changes** (Δsteering, Δspeed) based on recent sensor history.
- It runs entirely on the ESP32-S3, using optimized inference with **ESP-DL** and quantization via **ESP-PPQ**.
- Sensor data is normalized and fed into the model in real time, producing smooth and adaptive driving behavior (in most scenarios 😅. Need 
improvements).

---

## 🚫 Model Training & Files

- 🧪 **Model training scripts and datasets are NOT included**.
- 📦 The quantized model file is also excluded from version control.
-    I will include that with a little more improvements. Soon...!.
.

---

## 🛠️ Built With

- [ESP-IDF](https://github.com/espressif/esp-idf)
- [ESP-DL](https://github.com/espressif/esp-dl)
- [ESP-PPQ](https://github.com/espressif/esp-ppq)
- VL53L0X Time-of-Flight Sensors
- Custom motor driver logic

---

## 👀 What's Next?

- Explore new tracks and environmental variations.
- Balance data collection to improve bidirectional performance.
- Test alternative lightweight architectures for better edge performance.

---