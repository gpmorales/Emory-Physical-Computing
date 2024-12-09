#include <IRremote.hpp>

/**
 * Smart Outlet Control with IR Remote and Sound Detection
 * @Author: George Morales
 */

// Pin Definitions
const int SOUND_SENSOR = A0;    // LM393 Sound Sensor connected to analog pin A0
const int RELAY = 4;            // Relay power pin
const int IR_RECEIVE_PIN = 5;   // IR receiver pin

// State Variables
boolean OUTLET_STATUS = false;   // Tracks the current outlet status (ON/OFF)

// Sound Sensor (Analog Mode) Variables
int CALIBRATION_TIME = 5000;          // Time for initial calibration (ms)
boolean SOUND_ACTIVATED = false;      // Tracks if the sensor has been activated
const int SENSITIVITY = 7;            // Deviation threshold for sound detection
const int NOISE_THRESH = 20;          // Noise threshold for sensor activation
int clap_threshold_positive = 100;    // Positive spike threshold (calibrated value)
int clap_threshold_negative = 100;    // Negative spike threshold (calibrated value)

// Sound Sensor (Digital Mode) Variables
int lastSoundSensorState = LOW;       // Tracks the previous state of the digital sound sensor


// Setup Function
void setup() {
    Serial.begin(9600);

    // Configure pins
    pinMode(RELAY, OUTPUT);

    // Initialize infrared sensor module
    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
    Serial.println("IR Receiver Ready");
}

// Main Loop
void loop() {
    // Handle analog sound sensor input
    analogSoundSensor();

    // Handle IR-based control
    if (IrReceiver.decode()) {
        if (IrReceiver.decodedIRData.command == 0x40) { // 'Power' button command
            toggleOutlet();
            delay(100); // Debounce delay for IR input
        }
        // Print IR data and resume receiver
        IrReceiver.printIRResultShort(&Serial);
        IrReceiver.resume();
    }

    // Delay to prevent analog spikes
    delay(50);
}


// Analog Sound Sensor Handling
void analogSoundSensor() {
    // Read analog value from sound sensor
    int sound_sensor_output = analogRead(SOUND_SENSOR);

    // Calibrate sound sensor on first activation
    if (sound_sensor_output > NOISE_THRESH && !SOUND_ACTIVATED) {
        calibrateSoundSensor(CALIBRATION_TIME);
        SOUND_ACTIVATED = true;
    }

    Serial.println(sound_sensor_output);

    // Detect sound peaks/spikes
    if (sound_sensor_output > clap_threshold_positive ||
        (sound_sensor_output > NOISE_THRESH && sound_sensor_output < clap_threshold_negative)) {
        toggleOutlet();
        delay(100); // Short delay for debounce
        calibrateSoundSensor(500); // Recalibrate after toggling
    }
}

// Digital Sound Sensor Handling
void digitalSoundSensor() {
    // Read digital value from sound sensor
    int currentSoundSensorState = digitalRead(SOUND_SENSOR);

    Serial.println(currentSoundSensorState);

    // Detect rising edge (LOW to HIGH transition)
    if (currentSoundSensorState == HIGH && lastSoundSensorState == LOW) {
        toggleOutlet(); // Toggle outlet on sound detection
    }

    // Update the last state for edge detection
    lastSoundSensorState = currentSoundSensorState;

    // Delay to reduce rapid toggling
    delay(50);
}


/**
 * Toggles the outlet state
 */
void toggleOutlet() {
    OUTLET_STATUS = !OUTLET_STATUS;
    if (OUTLET_STATUS == true) {
      digitalWrite(RELAY, LOW);
    } else {
      digitalWrite(RELAY, HIGH);
    }
}


/**
 * Calibrates the sound sensor threshold
 * Runs for 5 seconds to establish baseline threshold of analog values
 */
void calibrateSoundSensor(int time) {
    long sum = 0;
    int num_readings = 0;
    unsigned long start_time = millis();
    
    Serial.println("Calibrating sound sensor...");
    
    while (millis() - start_time < time) {
        sum += analogRead(SOUND_SENSOR);
        num_readings++;
        delay(10);
    }
    
    int average = sum / num_readings;
    
    clap_threshold_positive = average + SENSITIVITY;
    clap_threshold_negative = average - SENSITIVITY;
  
    Serial.print("Calibration complete. Thresholds set to: ");
    Serial.println(clap_threshold_positive);
}
