/*
 * IRIS - Intelligent Robotic Integrated System
 * Main Controller File
 * 
 * Hardware: ESP32
 * Purpose: Command-based mobile robot for safe indoor delivery
 * 
 
 * Date: January 2026
 
 */

#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// ============================================================================
// CONFIGURATION & CONSTANTS
// ============================================================================

// WiFi Credentials
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Robot States
enum RobotState {
  IDLE,
  IN_TRANSIT,
  DELIVERED,
  RETURNING,
  STOPPED,
  ERROR_STATE
};

// Motor Pins (L298N Configuration)
const int MOTOR_FL_PIN1 = 13;  // Front Left Motor
const int MOTOR_FL_PIN2 = 12;
const int MOTOR_FR_PIN1 = 14;  // Front Right Motor
const int MOTOR_FR_PIN2 = 27;
const int MOTOR_BL_PIN1 = 26;  // Back Left Motor
const int MOTOR_BL_PIN2 = 25;
const int MOTOR_BR_PIN1 = 33;  // Back Right Motor
const int MOTOR_BR_PIN2 = 32;

const int MOTOR_ENA = 15;      // Enable A (Left Motors)
const int MOTOR_ENB = 2;       // Enable B (Right Motors)

// RFID Pins (MFRC522)
const int RFID_SS_PIN = 5;
const int RFID_RST_PIN = 22;

// Servo Pin
const int SERVO_PIN = 18;
const int SERVO_OPEN_ANGLE = 90;
const int SERVO_CLOSED_ANGLE = 0;

// PWM Configuration
const int PWM_FREQ = 1000;
const int PWM_RESOLUTION = 8;
const int PWM_CHANNEL_A = 0;
const int PWM_CHANNEL_B = 1;

// Authorized RFID Tags (UIDs in hex)
const String AUTHORIZED_TAGS[] = {
  "04 A1 B2 C3",
  "04 D4 E5 F6",
  "04 78 90 AB"
};
const int NUM_AUTHORIZED_TAGS = 3;

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

RobotState currentState = IDLE;
String currentTaskID = "";
String selectedPath = "";
bool returnToSource = false;
bool emergencyStopFlag = false;
bool requiresRFIDAuth = false;
unsigned long startTime = 0;
unsigned long deliveryTime = 0;
int currentWaypointIndex = 0;

// Hardware Objects
WebServer server(80);
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
Servo containerServo;
Preferences preferences;

// ============================================================================
// WAYPOINT STRUCTURE
// ============================================================================

struct Waypoint {
  String direction;  // FORWARD, BACKWARD, LEFT, RIGHT, STOP
  float distance;    // In cm or time units
  int speed;         // PWM value 0-255
  float angle;       // For turns, in degrees
};

// Predefined Paths
struct Path {
  String id;
  String source;
  String destination;
  Waypoint waypoints[20];
  int waypointCount;
};

Path predefinedPaths[10];
int pathCount = 0;

// ============================================================================
// SETUP FUNCTION
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== IRIS System Starting ===");
  
  // Initialize hardware
  initializeMotors();
  initializeRFID();
  initializeServo();
  initializeWiFi();
  initializeWebServer();
  initializePaths();
  
  // Initialize preferences for logging
  preferences.begin("iris-logs", false);
  
  // Set initial state
  currentState = IDLE;
  containerServo.write(SERVO_CLOSED_ANGLE);
  
  Serial.println("=== IRIS System Ready ===\n");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Handle web requests
  server.handleClient();
  
  // Check for emergency stop
  checkEmergencyStop();
  
  // System health monitoring
  if (millis() % 10000 == 0) {
    systemHealthCheck();
  }
  
  delay(10);
}

// ============================================================================
// INITIALIZATION FUNCTIONS
// ============================================================================

void initializeMotors() {
  Serial.println("Initializing motors...");
  
  // Configure motor pins
  pinMode(MOTOR_FL_PIN1, OUTPUT);
  pinMode(MOTOR_FL_PIN2, OUTPUT);
  pinMode(MOTOR_FR_PIN1, OUTPUT);
  pinMode(MOTOR_FR_PIN2, OUTPUT);
  pinMode(MOTOR_BL_PIN1, OUTPUT);
  pinMode(MOTOR_BL_PIN2, OUTPUT);
  pinMode(MOTOR_BR_PIN1, OUTPUT);
  pinMode(MOTOR_BR_PIN2, OUTPUT);
  
  // Configure PWM for motor speed control
  ledcSetup(PWM_CHANNEL_A, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_ENA, PWM_CHANNEL_A);
  ledcAttachPin(MOTOR_ENB, PWM_CHANNEL_B);
  
  stopMotors();
  Serial.println("Motors initialized");
}

void initializeRFID() {
  Serial.println("Initializing RFID...");
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("RFID initialized");
}

void initializeServo() {
  Serial.println("Initializing servo...");
  containerServo.attach(SERVO_PIN);
  containerServo.write(SERVO_CLOSED_ANGLE);
  Serial.println("Servo initialized");
}

void initializeWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void initializeWebServer() {
  Serial.println("Initializing web server...");
  
  // Define routes
  server.on("/", handleRoot);
  server.on("/status", handleGetStatus);
  server.on("/start", HTTP_POST, handleStartTask);
  server.on("/stop", HTTP_POST, handleEmergencyStop);
  server.on("/paths", handleGetPaths);
  server.on("/logs", handleGetLogs);
  
  server.begin();
  Serial.println("Web server started");
}

void initializePaths() {
  Serial.println("Initializing predefined paths...");
  
  // Example Path 1: Lab A to Lab B
  Path path1;
  path1.id = "PATH_1";
  path1.source = "Lab A";
  path1.destination = "Lab B";
  path1.waypointCount = 5;
  
  path1.waypoints[0] = {"FORWARD", 200, 200, 0};
  path1.waypoints[1] = {"RIGHT", 0, 150, 90};
  path1.waypoints[2] = {"FORWARD", 150, 200, 0};
  path1.waypoints[3] = {"LEFT", 0, 150, 90};
  path1.waypoints[4] = {"FORWARD", 100, 200, 0};
  
  predefinedPaths[pathCount++] = path1;
  
  // Example Path 2: Lab B to Storage
  Path path2;
  path2.id = "PATH_2";
  path2.source = "Lab B";
  path2.destination = "Storage";
  path2.waypointCount = 3;
  
  path2.waypoints[0] = {"FORWARD", 300, 200, 0};
  path2.waypoints[1] = {"RIGHT", 0, 150, 90};
  path2.waypoints[2] = {"FORWARD", 200, 200, 0};
  
  predefinedPaths[pathCount++] = path2;
  
  Serial.printf("Initialized %d paths\n", pathCount);
}

// ============================================================================
// WEB SERVER HANDLERS
// ============================================================================

void handleRoot() {
  String html = getControlPanelHTML();
  server.send(200, "text/html", html);
}

void handleGetStatus() {
  StaticJsonDocument<512> doc;
  
  doc["state"] = stateToString(currentState);
  doc["taskID"] = currentTaskID;
  doc["startTime"] = startTime;
  doc["deliveryTime"] = deliveryTime;
  doc["ipAddress"] = WiFi.localIP().toString();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleStartTask() {
  if (currentState != IDLE) {
    server.send(400, "application/json", "{\"error\":\"Robot is busy\"}");
    return;
  }
  
  String body = server.arg("plain");
  StaticJsonDocument<256> doc;
  deserializeJson(doc, body);
  
  selectedPath = doc["pathID"].as<String>();
  returnToSource = doc["returnFlag"].as<bool>();
  requiresRFIDAuth = doc["authRequired"].as<bool>();
  
  startNewTask();
  server.send(200, "application/json", "{\"status\":\"Task started\"}");
}

void handleEmergencyStop() {
  emergencyStop();
  server.send(200, "application/json", "{\"status\":\"Emergency stop activated\"}");
}

void handleGetPaths() {
  StaticJsonDocument<2048> doc;
  JsonArray pathsArray = doc.createNestedArray("paths");
  
  for (int i = 0; i < pathCount; i++) {
    JsonObject pathObj = pathsArray.createNestedObject();
    pathObj["id"] = predefinedPaths[i].id;
    pathObj["source"] = predefinedPaths[i].source;
    pathObj["destination"] = predefinedPaths[i].destination;
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleGetLogs() {
  // Retrieve logs from preferences
  String logs = preferences.getString("task_logs", "[]");
  server.send(200, "application/json", logs);
}

// ============================================================================
// TASK MANAGEMENT
// ============================================================================

void startNewTask() {
  // Generate task ID
  currentTaskID = "TASK_" + String(millis()) + "_" + String(random(1000, 9999));
  startTime = millis();
  currentWaypointIndex = 0;
  emergencyStopFlag = false;
  
  // Log task
  logTask(currentTaskID, selectedPath, startTime);
  
  // Update state
  currentState = IN_TRANSIT;
  Serial.printf("Task %s started on path %s\n", currentTaskID.c_str(), selectedPath.c_str());
  
  // Execute path
  executePath();
}

void executePath() {
  // Find the selected path
  Path* path = nullptr;
  for (int i = 0; i < pathCount; i++) {
    if (predefinedPaths[i].id == selectedPath) {
      path = &predefinedPaths[i];
      break;
    }
  }
  
  if (path == nullptr) {
    Serial.println("Error: Path not found");
    currentState = ERROR_STATE;
    return;
  }
  
  // Execute waypoints
  while (currentWaypointIndex < path->waypointCount && !emergencyStopFlag) {
    Waypoint wp = path->waypoints[currentWaypointIndex];
    Serial.printf("Executing waypoint %d: %s\n", currentWaypointIndex, wp.direction.c_str());
    
    navigateToWaypoint(wp);
    currentWaypointIndex++;
    delay(100);
  }
  
  if (emergencyStopFlag) {
    handleEmergencyStopState();
  } else {
    handleArrivalAtDestination();
  }
}

void navigateToWaypoint(Waypoint wp) {
  if (wp.direction == "FORWARD") {
    moveForward(wp.speed, wp.distance);
  } else if (wp.direction == "BACKWARD") {
    moveBackward(wp.speed, wp.distance);
  } else if (wp.direction == "LEFT") {
    turnLeft(wp.speed, wp.angle);
  } else if (wp.direction == "RIGHT") {
    turnRight(wp.speed, wp.angle);
  } else if (wp.direction == "STOP") {
    stopMotors();
  }
}

// ============================================================================
// MOTOR CONTROL
// ============================================================================

void moveForward(int speed, float distance) {
  setMotorSpeed(speed);
  
  digitalWrite(MOTOR_FL_PIN1, HIGH);
  digitalWrite(MOTOR_FL_PIN2, LOW);
  digitalWrite(MOTOR_FR_PIN1, HIGH);
  digitalWrite(MOTOR_FR_PIN2, LOW);
  digitalWrite(MOTOR_BL_PIN1, HIGH);
  digitalWrite(MOTOR_BL_PIN2, LOW);
  digitalWrite(MOTOR_BR_PIN1, HIGH);
  digitalWrite(MOTOR_BR_PIN2, LOW);
  
  unsigned long duration = calculateTimeForDistance(distance, speed);
  delay(duration);
  stopMotors();
}

void moveBackward(int speed, float distance) {
  setMotorSpeed(speed);
  
  digitalWrite(MOTOR_FL_PIN1, LOW);
  digitalWrite(MOTOR_FL_PIN2, HIGH);
  digitalWrite(MOTOR_FR_PIN1, LOW);
  digitalWrite(MOTOR_FR_PIN2, HIGH);
  digitalWrite(MOTOR_BL_PIN1, LOW);
  digitalWrite(MOTOR_BL_PIN2, HIGH);
  digitalWrite(MOTOR_BR_PIN1, LOW);
  digitalWrite(MOTOR_BR_PIN2, HIGH);
  
  unsigned long duration = calculateTimeForDistance(distance, speed);
  delay(duration);
  stopMotors();
}

void turnLeft(int speed, float angle) {
  setMotorSpeed(speed);
  
  // Right motors forward, left motors backward
  digitalWrite(MOTOR_FL_PIN1, LOW);
  digitalWrite(MOTOR_FL_PIN2, HIGH);
  digitalWrite(MOTOR_FR_PIN1, HIGH);
  digitalWrite(MOTOR_FR_PIN2, LOW);
  digitalWrite(MOTOR_BL_PIN1, LOW);
  digitalWrite(MOTOR_BL_PIN2, HIGH);
  digitalWrite(MOTOR_BR_PIN1, HIGH);
  digitalWrite(MOTOR_BR_PIN2, LOW);
  
  unsigned long duration = calculateTimeForAngle(angle, speed);
  delay(duration);
  stopMotors();
}

void turnRight(int speed, float angle) {
  setMotorSpeed(speed);
  
  // Left motors forward, right motors backward
  digitalWrite(MOTOR_FL_PIN1, HIGH);
  digitalWrite(MOTOR_FL_PIN2, LOW);
  digitalWrite(MOTOR_FR_PIN1, LOW);
  digitalWrite(MOTOR_FR_PIN2, HIGH);
  digitalWrite(MOTOR_BL_PIN1, HIGH);
  digitalWrite(MOTOR_BL_PIN2, LOW);
  digitalWrite(MOTOR_BR_PIN1, LOW);
  digitalWrite(MOTOR_BR_PIN2, HIGH);
  
  unsigned long duration = calculateTimeForAngle(angle, speed);
  delay(duration);
  stopMotors();
}

void stopMotors() {
  digitalWrite(MOTOR_FL_PIN1, LOW);
  digitalWrite(MOTOR_FL_PIN2, LOW);
  digitalWrite(MOTOR_FR_PIN1, LOW);
  digitalWrite(MOTOR_FR_PIN2, LOW);
  digitalWrite(MOTOR_BL_PIN1, LOW);
  digitalWrite(MOTOR_BL_PIN2, LOW);
  digitalWrite(MOTOR_BR_PIN1, LOW);
  digitalWrite(MOTOR_BR_PIN2, LOW);
}

void setMotorSpeed(int speed) {
  ledcWrite(PWM_CHANNEL_A, speed);
  ledcWrite(PWM_CHANNEL_B, speed);
}

// ============================================================================
// DELIVERY HANDLING
// ============================================================================

void handleArrivalAtDestination() {
  stopMotors();
  deliveryTime = millis();
  currentState = DELIVERED;
  
  unsigned long taskDuration = deliveryTime - startTime;
  Serial.printf("Arrived at destination. Duration: %lu ms\n", taskDuration);
  
  logTaskCompletion(currentTaskID, deliveryTime, taskDuration);
  
  if (requiresRFIDAuth) {
    waitForRFIDAuthentication();
  }
  
  if (returnToSource) {
    delay(5000);
    returnToSourceLocation();
  } else {
    currentState = IDLE;
    currentTaskID = "";
    Serial.println("Task complete");
  }
}

// ============================================================================
// RFID AUTHENTICATION
// ============================================================================

void waitForRFIDAuthentication() {
  Serial.println("Waiting for RFID authentication...");
  unsigned long authStartTime = millis();
  unsigned long authTimeout = 300000; // 5 minutes
  bool authenticated = false;
  
  while ((millis() - authStartTime) < authTimeout && !authenticated) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String tagID = getCardUID();
      Serial.printf("RFID Tag detected: %s\n", tagID.c_str());
      
      if (isAuthorizedTag(tagID)) {
        authenticated = true;
        unlockContainer();
        logAccess(currentTaskID, tagID, "GRANTED");
        Serial.println("Access granted");
      } else {
        logAccess(currentTaskID, tagID, "DENIED");
        Serial.println("Access denied");
        delay(2000);
      }
      
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
    delay(100);
  }
  
  if (!authenticated) {
    Serial.println("Authentication timeout");
    logAccess(currentTaskID, "TIMEOUT", "TIMEOUT");
  }
}

String getCardUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();
  return uid;
}

bool isAuthorizedTag(String tagID) {
  for (int i = 0; i < NUM_AUTHORIZED_TAGS; i++) {
    if (tagID == AUTHORIZED_TAGS[i]) {
      return true;
    }
  }
  return false;
}

void unlockContainer() {
  containerServo.write(SERVO_OPEN_ANGLE);
  Serial.println("Container unlocked");
  delay(30000); // 30 seconds to remove contents
  lockContainer();
}

void lockContainer() {
  containerServo.write(SERVO_CLOSED_ANGLE);
  Serial.println("Container locked");
}

// ============================================================================
// RETURN TO SOURCE
// ============================================================================

void returnToSourceLocation() {
  currentState = RETURNING;
  Serial.println("Returning to source...");
  
  // Find the path and reverse it
  Path* path = nullptr;
  for (int i = 0; i < pathCount; i++) {
    if (predefinedPaths[i].id == selectedPath) {
      path = &predefinedPaths[i];
      break;
    }
  }
  
  if (path == nullptr) {
    Serial.println("Error: Cannot find path for return");
    currentState = ERROR_STATE;
    return;
  }
  
  // Execute waypoints in reverse
  for (int i = path->waypointCount - 1; i >= 0 && !emergencyStopFlag; i--) {
    Waypoint wp = invertWaypoint(path->waypoints[i]);
    navigateToWaypoint(wp);
    delay(100);
  }
  
  if (!emergencyStopFlag) {
    stopMotors();
    Serial.println("Returned to source");
    logReturn(currentTaskID, millis());
    currentState = IDLE;
    currentTaskID = "";
  }
}

Waypoint invertWaypoint(Waypoint wp) {
  Waypoint inverted = wp;
  
  if (wp.direction == "FORWARD") {
    inverted.direction = "BACKWARD";
  } else if (wp.direction == "BACKWARD") {
    inverted.direction = "FORWARD";
  } else if (wp.direction == "LEFT") {
    inverted.direction = "RIGHT";
  } else if (wp.direction == "RIGHT") {
    inverted.direction = "LEFT";
  }
  
  return inverted;
}

// ============================================================================
// EMERGENCY STOP
// ============================================================================

void emergencyStop() {
  emergencyStopFlag = true;
  stopMotors();
  currentState = STOPPED;
  Serial.println("EMERGENCY STOP ACTIVATED");
  logEmergencyStop(currentTaskID, millis());
}

void handleEmergencyStopState() {
  Serial.println("Task halted - Manual intervention required");
  // Robot remains stopped until reset
}

void checkEmergencyStop() {
  // Check for emergency stop conditions
  // This could be a physical button, web command, or sensor trigger
  if (emergencyStopFlag && currentState != STOPPED) {
    emergencyStop();
  }
}

// ============================================================================
// LOGGING FUNCTIONS
// ============================================================================

void logTask(String taskID, String pathID, unsigned long startTime) {
  Serial.printf("LOG: Task %s started on path %s at %lu\n", 
                taskID.c_str(), pathID.c_str(), startTime);
  // Store in preferences or SD card
}

void logTaskCompletion(String taskID, unsigned long deliveryTime, unsigned long duration) {
  Serial.printf("LOG: Task %s completed at %lu (duration: %lu ms)\n", 
                taskID.c_str(), deliveryTime, duration);
}

void logAccess(String taskID, String tagID, String result) {
  Serial.printf("LOG: Access attempt for task %s - Tag: %s - Result: %s\n", 
                taskID.c_str(), tagID.c_str(), result.c_str());
}

void logReturn(String taskID, unsigned long returnTime) {
  Serial.printf("LOG: Task %s returned at %lu\n", taskID.c_str(), returnTime);
}

void logEmergencyStop(String taskID, unsigned long stopTime) {
  Serial.printf("LOG: Emergency stop for task %s at %lu\n", taskID.c_str(), stopTime);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

unsigned long calculateTimeForDistance(float distance, int speed) {
  // Calibration needed: milliseconds per cm at given speed
  // This is a placeholder - adjust based on your robot's performance
  float timePerCm = 1000.0 / speed;
  return (unsigned long)(distance * timePerCm);
}

unsigned long calculateTimeForAngle(float angle, int speed) {
  // Calibration needed: milliseconds per degree at given speed
  // This is a placeholder - adjust based on your robot's turning performance
  float timePerDegree = 50.0 / (speed / 100.0);
  return (unsigned long)(angle * timePerDegree);
}

String stateToString(RobotState state) {
  switch (state) {
    case IDLE: return "IDLE";
    case IN_TRANSIT: return "IN_TRANSIT";
    case DELIVERED: return "DELIVERED";
    case RETURNING: return "RETURNING";
    case STOPPED: return "STOPPED";
    case ERROR_STATE: return "ERROR";
    default: return "UNKNOWN";
  }
}

void systemHealthCheck() {
  // Monitor WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WARNING: WiFi disconnected");
  }
  
  // Monitor memory
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
}

String getControlPanelHTML() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>IRIS Control Panel</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; margin: 20px; background: #f0f0f0; }
    .container { max-width: 800px; margin: auto; background: white; padding: 20px; border-radius: 8px; }
    h1 { color: #333; }
    .status { padding: 15px; margin: 10px 0; border-radius: 5px; font-weight: bold; }
    .idle { background: #4CAF50; color: white; }
    .transit { background: #2196F3; color: white; }
    .delivered { background: #FF9800; color: white; }
    .stopped { background: #f44336; color: white; }
    button { padding: 10px 20px; margin: 5px; font-size: 16px; cursor: pointer; border-radius: 5px; border: none; }
    .start-btn { background: #4CAF50; color: white; }
    .stop-btn { background: #f44336; color: white; }
    select { padding: 8px; margin: 10px 0; width: 100%; font-size: 14px; }
    .info { margin: 10px 0; padding: 10px; background: #e3f2fd; border-radius: 5px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>🤖 IRIS Control Panel</h1>
    <div class="status idle" id="status">Status: IDLE</div>
    <div class="info">
      <p><strong>Task ID:</strong> <span id="taskId">-</span></p>
      <p><strong>IP Address:</strong> <span id="ipAddr">-</span></p>
    </div>
    
    <h3>Start New Task</h3>
    <select id="pathSelect">
      <option value="">Select Path...</option>
    </select>
    <br>
    <label><input type="checkbox" id="returnCheck"> Return to source</label>
    <br>
    <label><input type="checkbox" id="authCheck"> Require RFID authentication</label>
    <br><br>
    <button class="start-btn" onclick="startTask()">Start Task</button>
    <button class="stop-btn" onclick="stopTask()">Emergency Stop</button>
    
    <h3>Activity Log</h3>
    <div id="log" style="max-height: 200px; overflow-y: auto; background: #fafafa; padding: 10px; border-radius: 5px;"></div>
  </div>
  
  <script>
    function updateStatus() {
      fetch('/status')
        .then(r => r.json())
        .then(data => {
          document.getElementById('status').textContent = 'Status: ' + data.state;
          document.getElementById('taskId').textContent = data.taskID || '-';
          document.getElementById('ipAddr').textContent = data.ipAddress || '-';
          
          let statusDiv = document.getElementById('status');
          statusDiv.className = 'status ' + 
            (data.state === 'IDLE' ? 'idle' : 
             data.state === 'IN_TRANSIT' || data.state === 'RETURNING' ? 'transit' : 
             data.state === 'DELIVERED' ? 'delivered' : 'stopped');
        });
    }
    
    function loadPaths() {
      fetch('/paths')
        .then(r => r.json())
        .then(data => {
          let select = document.getElementById('pathSelect');
          data.paths.forEach(path => {
            let option = document.createElement('option');
            option.value = path.id;
            option.textContent = path.source + ' → ' + path.destination;
            select.appendChild(option);
          });
        });
    }
    
    function startTask() {
      let pathID = document.getElementById('pathSelect').value;
      if (!pathID) {
        alert('Please select a path');
        return;
      }
      
      fetch('/start', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({
          pathID: pathID,
          returnFlag: document.getElementById('returnCheck').checked,
          authRequired: document.getElementById('authCheck').checked
        })
      })
      .then(r => r.json())
      .then(data => {
        addLog('Task started: ' + pathID);
        updateStatus();
      });
    }
    
    function stopTask() {
      if (confirm('Activate emergency stop?')) {
        fetch('/stop', {method: 'POST'})
          .then(() => {
            addLog('Emergency stop activated');
            updateStatus();
          });
      }
    }
    
    function addLog(message) {
      let log = document.getElementById('log');
      let time = new Date().toLocaleTimeString();
      log.innerHTML = `<div>[${time}] ${message}</div>` + log.innerHTML;
    }
    
    // Initialize
    loadPaths();
    updateStatus();
    setInterval(updateStatus, 2000);
  </script>
</body>
</html>
)rawliteral";
}
