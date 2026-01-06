**🤖 IRIS – Intelligent Robotic Integrated System**

IRIS (Intelligent Robotic Integrated System) is a command-based, IoT-enabled autonomous mobile robot designed for safe and contactless indoor material transportation.
The system is developed mainly for restaurants, hospitals, and laboratories, where repetitive delivery tasks or hazardous material handling are required.

The project focuses on controlled movement, safety, and authorization, rather than complex AI navigation.

**📌 Problem Statement**

In environments like hospitals, labs, and restaurants:

Human staff repeatedly transport items between fixed points

There is risk of contamination, chemical exposure, and human error

Many delivery paths are fixed and predictable

IRIS is designed to reduce human involvement in such repetitive and risky tasks by providing a safe, controlled, and repeatable robotic delivery system.

**💡 Solution Overview**

IRIS operates as a web-controlled robotic system where:

An operator selects a predefined path from a web interface

The robot moves from Point A to Point B

Delivery is confirmed digitally

Optional RFID authentication ensures only authorized users can access contents

The robot can return to source automatically

The system is modular, meaning the same robot can be adapted for:

Food delivery (restaurant)

Sterilized tools transport (hospital)

Hazardous material handling (laboratory)

**⚙️ System Architecture (High Level)**
Web Interface (HTML)
        ↓
ESP32 Web Server
        ↓
Task Controller & State Machine
        ↓
Motor Driver (L298N)
        ↓
Robot Movement + Sensors

**🧠 Core Features**
🔹 Command-Based Operation

Tasks are sent from a web-based control panel

Robot states:

IDLE

IN_TRANSIT

DELIVERED

RETURNING

STOPPED

ERROR

**🔹 Predefined Path Navigation**

Robot follows stored waypoints

Suitable for indoor environments with fixed layouts

No dependency on GPS or complex mapping

**🔹 Web Control Panel (ESP32 Hosted)**

Built-in ESP32 web server

Allows:

Path selection

Task start / stop

Return option

RFID authorization toggle

Live status monitoring

**🔹 Secure RFID-Based Access Control**

MFRC522 RFID module

Only authorized RFID tags can unlock the container

Servo motor controls lid opening and closing

Used mainly for:

Medical tools

Hazardous materials

**🔹 Isolated Container with UV Disinfection (Planned)**

Sealed container accessory

UV light for sterilization of contents

Designed for hospital and laboratory use

**🔹 Emergency Stop System**

Can be triggered:

From web interface

Internally by system conditions

Immediately halts motors

Logs emergency event

**🔹 Task Logging**

Task ID generation

Start time, delivery time, duration

Stored using ESP32 Preferences

Helps in monitoring and debugging

**🧪 Safety Features (Planned & Expandable)**

Gas sensor for chemical leakage detection

Buzzer alert when hazardous materials are approached

Servo-based radar (180° sweep) for obstacle awareness

Future scope: camera-based detection and AI navigation

**🧩 Hardware Components**

Component	Purpose
ESP32	Main controller + Wi-Fi
L298N Motor Driver	Motor control
DC Motors	Robot movement
MFRC522 RFID	Authentication
Servo Motor	Lid control / radar
Ultrasonic Sensor	Obstacle detection
Gas Sensor	Chemical leakage detection
Buzzer	Safety alerts
UV LED	Sterilization
Battery Pack	Power supply

**🧑‍💻 Software & Libraries Used**

Arduino Framework

ESP32 WiFi

WebServer

ArduinoJson

MFRC522 (RFID)

ESP32Servo

Preferences (non-volatile storage)

**🌐 Web API Endpoints**

Endpoint	Method	Description
/	GET	Control panel
/status	GET	Robot status
/start	POST	Start task
/stop	POST	Emergency stop
/paths	GET	Available paths
/logs	GET	Task logs

**🚀 How to Run**

Install Arduino IDE

Install ESP32 board support

Install required libraries

Update WiFi credentials in code

Upload code to ESP32

Open Serial Monitor to get IP address

Open IP address in browser

**📈 Future Scope**

Camera-based navigation

AI-based obstacle avoidance

Mobile app integration

Cloud logging and analytics

Multi-robot coordination

**👨‍🎓 Project Status**

ESP32 hardware tested

Web interface working

Motor control logic implemented

RFID authentication implemented

Modular design ready for expansion
