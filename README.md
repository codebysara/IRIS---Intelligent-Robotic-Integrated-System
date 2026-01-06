# IRIS---Intelligent-Robotic-Integrated-System
A multipurpose IoT based autonomous robot designed for contactless material transport in restaurants, hospitals , and laboratories.

**Overall Idea**

IRIS is an indoor mobile robot used to move items from one fixed point to another without human involvement. The robot is controlled using a simple web page. The user selects the destination and starts the task. The robot follows predefined paths and avoids obstacles using sensors. The same robot is used for different environments by changing the container and enabling required features.

**Restaurant Application**

For restaurant use, IRIS carries food items from the kitchen to tables or service areas. A simple open tray is placed on the robot. The operator selects the destination using the web interface and starts the robot. The robot moves along a fixed path and stops if any obstacle is detected. A radar-based rotating sensor scans the front area to detect people or objects early. After delivering the food, the robot can return to the base.

**Hospital Application (Isolated Box + UV Light)**

For hospital use, the tray is replaced with an isolated box. This box is closed from all sides to protect the contents. Inside the box, a UV light is placed. The UV light helps in keeping medical tools and medicines disinfected during transport. The box opens only when an authorized staff member scans an RFID card. This ensures that only trained personnel can access the contents. This setup reduces contamination and human contact.

**Laboratory Application (Isolated Box + Gas Detection)**

For laboratory use, the robot carries hazardous or corrosive chemicals. An airtight isolated box is used so that chemicals do not leak outside. A gas sensor is mounted near the container to continuously check for any leakage. If gas is detected, a buzzer starts ringing to warn nearby people. The radar sensor also checks if someone comes too close to the robot and gives a warning. The box opens only after RFID verification.

**Radar Scanning (Easy Explanation)**

IRIS uses a sensor mounted on a small motor that rotates up to 180 degrees. This works like a simple radar. It helps the robot see obstacles, people, or objects in a wider area in front of it. This makes movement safer, especially in crowded indoor spaces.

**Control and Safety**

The robot is controlled using a basic HTML web page. The current state of the robot such as IDLE, MOVING, or DELIVERED is shown on the screen. A STOP button is provided for emergencies. Even if the robot is controlled online, safety features like obstacle detection, gas detection, and proximity alerts always work locally.

**Final Understanding**

IRIS is a simple, modular robot. By changing the container and enabling specific features like UV light, gas sensing, or RFID, the same robot can be used in restaurants, hospitals, and laboratories. The project focuses on safety, simplicity, and repeatable indoor transportation.
