#include<Arduino.h>

// LSA08 Digital Port B Connections (using only 6 sensors)
#define S1 3
#define S2 4
#define S3 5
#define S4 6
#define S5 7
#define S6 8

// Motor Control Pins (L298N)
#define ENA 10
#define ENB 11
#define IN1 A0
#define IN2 A1
#define IN3 A2
#define IN4 A3

// LED and Constants
#define FINISH_LED 2
const int ALL_BLACK_STOP_TIME = 4000; // Time in ms to stop when all sensors detect black
unsigned long allBlackStartTime = 0;  // To track how long all sensors have been black
bool allBlackDetected = false;        // Flag to track if all black is detected
bool robotStopped = false;            // Flag to track if robot has stopped

// Speed settings
int MOTOR_SPEED = 95;  // Default speed, will be changed based on runtime
const int SLOW_SPEED = 60;  // Slower speed when approaching finish

void setup() {
  // Sensor Inputs
  pinMode(S1, INPUT);
  pinMode(S2, INPUT);
  pinMode(S3, INPUT);
  pinMode(S4, INPUT);
  pinMode(S5, INPUT);
  pinMode(S6, INPUT);

  // Motor Control Setup
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // Finish LED
  pinMode(FINISH_LED, OUTPUT);
}

void loop() {
  /*
  static unsigned long startTime = 0;
  static bool initialSpeedSet = false;

  if (!initialSpeedSet) {
    startTime = millis();
    initialSpeedSet = true;
  }

  // Change speed after 10 seconds
  if (millis() - startTime < 10000) {
    MOTOR_SPEED = 80;  // First 10 seconds: slower speed
  } else {
    MOTOR_SPEED = 120; // After 10 seconds: faster speed
  }
  */
  if(robotStopped) {
    return; // If robot is stopped, don't do anything else
  }

  // Check for all sensors detecting black (potential finish line)
  bool allSensorsActive = digitalRead(S1) && digitalRead(S2) && digitalRead(S3) &&
                          digitalRead(S4) && digitalRead(S5) && digitalRead(S6);

  // Handle all black detection for finish line
  if(handleAllBlackDetection(allSensorsActive)) {
    return; // Skip rest of loop if handling finish line
  }

  // Check for sharp turns first
  if(handleSharpTurns()) {
    return; // Skip rest of loop if taking a sharp turn
  }

  // Simple line following based on direct sensor readings
  followLineDirect();
}

bool handleAllBlackDetection(bool allSensorsActive) {
  if(allSensorsActive) {
    if(!allBlackDetected) {
      // First detection of all black
      allBlackDetected = true;
      allBlackStartTime = millis();
      
      // Slow down when all sensors detect black
      analogWrite(ENA, SLOW_SPEED);
      analogWrite(ENB, SLOW_SPEED);
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
    } else {
      // Check if all black has been detected for the required time
      if(millis() - allBlackStartTime >= ALL_BLACK_STOP_TIME) {
        // Stop the robot and light up the LED
        stopMotors();
        digitalWrite(FINISH_LED, HIGH);
        robotStopped = true;
      }
    }
    return true; // Handling all black detection
  } else {
    // Reset the all black detection if any sensor is not black
    allBlackDetected = false;
    return false; // Not handling all black detection
  }
}

bool handleSharpTurns() {
  // Read sensors
  bool s1 = digitalRead(S1);
  bool s2 = digitalRead(S2);
  bool s3 = digitalRead(S3);
  bool s4 = digitalRead(S4);
  bool s5 = digitalRead(S5);
  bool s6 = digitalRead(S6);
  
  // Check if only leftmost sensor(s) detect the line
  if ((s1 && !s6) || (s1 && s2 && !s6)) {
    // Sharp left turn until right sensor detects the line
    while (!(digitalRead(S6))) {
      // Turn left until center sensors detect the line
      analogWrite(ENA, 50);  // Very slow inside wheel
      analogWrite(ENB, 120); // Fast outside wheel
      digitalWrite(IN1, LOW);  // Reverse inside wheel for sharp turn
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      
      // Brief delay to avoid overwhelming the processor
      delay(10);
      
      // Check for all black (possible finish line during turn)
      if(digitalRead(S1) && digitalRead(S2) && digitalRead(S3) && 
         digitalRead(S4) && digitalRead(S5) && digitalRead(S6)) {
        return false; // Exit the turn to handle possible finish line
      }
    }
    return true;
  }
  
  // Check if only rightmost sensor(s) detect the line
  if ((s6 && !s1) || (s6 && s5 && !s1)) {
    // Sharp right turn until left sensor detects the line
    while (!(digitalRead(S1))) {
      // Turn right until center sensors detect the line
      analogWrite(ENA, 120); // Fast outside wheel
      analogWrite(ENB, 50);  // Very slow inside wheel
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW); // Reverse inside wheel for sharp turn
      digitalWrite(IN4, HIGH);
      
      // Brief delay to avoid overwhelming the processor
      delay(10);
      
      // Check for all black (possible finish line during turn)
      if(digitalRead(S1) && digitalRead(S2) && digitalRead(S3) && 
         digitalRead(S4) && digitalRead(S5) && digitalRead(S6)) {
        return false; // Exit the turn to handle possible finish line
      }
    }
    return true;
  }
  
  return false; // No sharp turn needed
}

void followLineDirect() {
  // Read all sensors
  bool s1 = digitalRead(S1);
  bool s2 = digitalRead(S2);
  bool s3 = digitalRead(S3);
  bool s4 = digitalRead(S4);
  bool s5 = digitalRead(S5);
  bool s6 = digitalRead(S6);
  
  // No line detected - search
  if(!s1 && !s2 && !s3 && !s4 && !s5 && !s6) {
    moveForward(MOTOR_SPEED);
    //searchLine();
    return;
  }
  
  // Simple line following logic based on direct sensor readings
  // Center sensors have priority
  else if(s3 || s4) {
    // Line is in the center - go straight
    moveForward(MOTOR_SPEED);
  }
  else if(s1 && s2 && s3 && s4 && s5 && s6){
    moveForward(MOTOR_SPEED);
  }
  else if(s2 || s1) {
    // Line is slightly to the left - gentle left turn
    turnLeft(75);
  }
  else if(s5 || s6) {
    // Line is slightly to the right - gentle right turn
    turnRight(75);
  }
}

void searchLine() {
  static unsigned long lastChangeTime = 0;
  static int searchDirection = 0;  // 0: left, 1: right
  
  // Change direction every 750ms
  if(millis() - lastChangeTime > 750) {
    searchDirection = !searchDirection;
    lastChangeTime = millis();
  }
  
  if(searchDirection == 0) {
    turnLeft(MOTOR_SPEED - 20);
  } else {
    turnRight(MOTOR_SPEED - 20);
  }
}

void moveForward(int speed) {
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnLeft(int speed) {
  analogWrite(ENA, speed - 30);  // Inside wheel slower
  analogWrite(ENB, speed);       // Outside wheel normal speed
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight(int speed) {
  analogWrite(ENA, speed);       // Outside wheel normal speed
  analogWrite(ENB, speed - 30);  // Inside wheel slower
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}