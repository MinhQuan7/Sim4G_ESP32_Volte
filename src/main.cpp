
/*
 * ESP32 with SIM A7683E Module - LTE Cat 1bis
 *
 * A7683E Features:
 * - LTE Cat 1bis with max download speed 10Mbps, upload 5Mbps
 * - Compatible with AT commands like SIM800 series
 * - Supports VoLTE, SMS, TCP/IP, HTTP/HTTPS, FTP, SSL
 * - LGA form factor compatible with SIM800C, SIM868, SIM7080G, A7682E
 *
 * Pin Configuration for A7683E:
 * - VCC: 3.4V ~ 4.2V (Typ: 3.8V)
 * - GND: Ground
 * - TX (GPIO 17): ESP32 -> A7683E (UART transmit)
 * - RX (GPIO 16): ESP32 <- A7683E (UART receive)
 * - PEN (GPIO 23): Power Enable - Turn on/off module power
 * - NET (GPIO 5): Network Status - Network status indicator
 * - DTR (GPIO 4): Data Terminal Ready - Sleep/Wake control
 *
 * Functions:
 * - PEN: HIGH = Turn on module power, LOW = Turn off power
 * - NET: HIGH = Network connected, LOW = No network
 * - DTR: LOW = Wake up, HIGH = Sleep mode
 */

#include <HardwareSerial.h>

// A7683E Module Pin Configuration
#define simSerial Serial2
#define MCU_SIM_BAUDRATE 115200
#define MCU_SIM_TX_PIN 17
#define MCU_SIM_RX_PIN 16
#define MCU_SIM_PEN_PIN 23 // Power Enable Pin
#define MCU_SIM_NET_PIN 5  // Network Status Pin
#define MCU_SIM_DTR_PIN 4  // Data Terminal Ready Pin
#define optocoupler_Sensor 19

#define PHONE_NUMBER "+84909483187" // Enter phone number

// Variables to track previous sensor state
bool lastSensorState = false;
unsigned long lastNotificationTime = 0;
const unsigned long NOTIFICATION_INTERVAL = 5000; // 5 seconds interval between repeated notifications

// Call limiting mechanism variables
int callCount = 0;                                // Number of calls made in current batch
const int MAX_CALLS_PER_BATCH = 3;                // Max 3 calls per batch
unsigned long callBatchStartTime = 0;             // When current batch started
unsigned long callCooldownStartTime = 0;          // When cooldown period started
bool inCallCooldown = false;                      // Whether we're in 1-minute cooldown
const unsigned long CALL_COOLDOWN_PERIOD = 60000; // 1 minute cooldown between batches

// Non-blocking timer variables
unsigned long lastSensorReadTime = 0;
unsigned long lastAtWaitTime = 0;
unsigned long smsStartTime = 0;
unsigned long callStartTime = 0;
bool smsInProgress = false;
bool callInProgress = false;
bool moduleInitialized = false;

// Timer intervals
const unsigned long SENSOR_READ_INTERVAL = 100; // Read sensor every 100ms (faster response)
const unsigned long AT_WAIT_INTERVAL = 100;     // AT wait interval
const unsigned long SMS_TIMEOUT = 10000;        // SMS timeout 10 seconds
const unsigned long CALL_DURATION = 20000;      // Call duration 20 seconds

// Function declarations (C++ requires forward declarations)
void sim_at_wait();
bool sim_at_cmd(String cmd);
bool sim_at_send(char c);
void init_a7683e_module();
bool check_network_status_pin();
void sleep_module();
void wakeup_module();
void power_off_module();
void power_on_module();
void monitor_net_status();
void diagnostic_sim();
bool check_network_status();
void reconnect_network();
void sent_sms(String message);
void call();
void readSensor();
void start_emergency_sms();
void start_emergency_call();
void handle_sms_process();
void handle_call_process();
void non_blocking_sim_at_wait();
void check_call_cooldown();

void readSensor()
{

  // Non-blocking sensor reading
  if (millis() - lastSensorReadTime >= SENSOR_READ_INTERVAL)
  {
    uint8_t valueOpto = digitalRead(optocoupler_Sensor);

    // Detailed debugging every 10 seconds
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime >= 10000)
    {
      Serial.println("=== SENSOR DEBUG ===");
      Serial.println("Digital reading: " + String(valueOpto));
      Serial.println("Last state: " + String(lastSensorState));
      Serial.println("===================");
      lastDebugTime = millis();
    }
    else
    {
      // Normal reading display
      Serial.println("Opto: " + String(valueOpto) + " (0=Power ON, 1=Power OFF)");
    }

    // Check for power loss (sensor = 1) and state change
    if (valueOpto == 1 && lastSensorState == false)
    {
      // ALWAYS notify immediately when power loss is first detected
      Serial.println("*** POWER LOSS DETECTED! ***");
      Serial.println("Transition: Power ON -> Power OFF");
      Serial.println("Starting emergency notification sequence...");

      // Start SMS notification (non-blocking)
      start_emergency_sms();

      lastNotificationTime = millis();
    }
    // Check for repeated power loss notifications (every 5 seconds while power is still lost)
    else if (valueOpto == 1 && lastSensorState == true)
    {
      // Only send repeated notifications if 5 seconds have passed
      if (millis() - lastNotificationTime > NOTIFICATION_INTERVAL)
      {
        Serial.println("*** POWER STILL LOST - REPEAT NOTIFICATION ***");
        Serial.println("Sending repeated emergency notification...");

        // Start SMS notification (non-blocking)
        start_emergency_sms();

        lastNotificationTime = millis();
      }
    }

    // Also detect power restoration for info
    else if (valueOpto == 0 && lastSensorState == true)
    {
      Serial.println("*** POWER RESTORED! ***");
      Serial.println("Transition: Power OFF -> Power ON");
    }

    // Update previous sensor state
    lastSensorState = (valueOpto == 1);
    lastSensorReadTime = millis();
  }
}

void sim_at_wait()
{
  // Non-blocking version - just read available data
  while (simSerial.available())
  {
    Serial.write(simSerial.read());
  }
}

void non_blocking_sim_at_wait()
{
  // Non-blocking AT response handler
  if (millis() - lastAtWaitTime >= AT_WAIT_INTERVAL)
  {
    while (simSerial.available())
    {
      Serial.write(simSerial.read());
    }
    lastAtWaitTime = millis();
  }
}

bool sim_at_cmd(String cmd)
{
  simSerial.println(cmd);
  sim_at_wait();
  return true;
}

bool sim_at_send(char c)
{
  simSerial.write(c);
  return true;
}

// Initialize A7683E Module
void init_a7683e_module()
{
  Serial.println("=== INITIALIZING A7683E MODULE ===");

  // 1. Configure GPIO pins
  pinMode(MCU_SIM_PEN_PIN, OUTPUT);
  pinMode(MCU_SIM_NET_PIN, INPUT);
  pinMode(MCU_SIM_DTR_PIN, OUTPUT);

  // 2. Turn on module power (PEN = HIGH)
  Serial.println("1. Turning on module power...");
  digitalWrite(MCU_SIM_PEN_PIN, HIGH);

  // 3. Wake up module (DTR = LOW)
  Serial.println("2. Waking up module...");
  digitalWrite(MCU_SIM_DTR_PIN, LOW);

  // 4. Module will be ready for use - no blocking delay
  Serial.println("3. Module initialization started...");
  Serial.println("=== INITIALIZATION COMPLETE ===");
}

// Check NET pin status
bool check_network_status_pin()
{
  bool net_status = digitalRead(MCU_SIM_NET_PIN);
  Serial.print("NET Pin Status: ");
  Serial.println(net_status ? "HIGH (Connected)" : "LOW (Disconnected)");
  return net_status;
}

// Put A7683E module to sleep
void sleep_module()
{
  Serial.println("Putting module into sleep mode...");
  digitalWrite(MCU_SIM_DTR_PIN, HIGH);
  Serial.println("Module entered sleep mode (DTR=HIGH)");
}

// Wake up A7683E module
void wakeup_module()
{
  Serial.println("Waking up module...");
  digitalWrite(MCU_SIM_DTR_PIN, LOW);
  Serial.println("Module woke up (DTR=LOW)");
}

// Power off A7683E module
void power_off_module()
{
  Serial.println("Turning off module power...");
  digitalWrite(MCU_SIM_PEN_PIN, LOW);
  Serial.println("Module power turned off (PEN=LOW)");
}

// Power on A7683E module
void power_on_module()
{
  Serial.println("Turning on module power...");
  digitalWrite(MCU_SIM_PEN_PIN, HIGH);
  Serial.println("Module power turned on (PEN=HIGH)");
}

// Monitor NET pin in real time
void monitor_net_status()
{
  static bool last_net_status = false;
  static unsigned long last_check = 0;

  if (millis() - last_check > 2000) // Check every 2 seconds
  {
    bool current_net_status = digitalRead(MCU_SIM_NET_PIN);

    if (current_net_status != last_net_status)
    {
      Serial.print("NET Status changed: ");
      Serial.println(current_net_status ? "CONNECTED" : "DISCONNECTED");
      last_net_status = current_net_status;
    }

    last_check = millis();
  }
}

// Detailed SIM and network diagnostics
void diagnostic_sim()
{
  Serial.println("=== SIM DIAGNOSTICS ===");

  // Ensure module is awake
  wakeup_module();

  // Check SIM card
  Serial.println("1. Checking SIM:");
  sim_at_cmd("AT+CPIN?");
  delay(1000);

  // SIM information
  Serial.println("2. IMSI:");
  sim_at_cmd("AT+CIMI");
  delay(1000);

  // ICCID information
  Serial.println("3. ICCID (SIM serial number):");
  sim_at_cmd("AT+CCID");
  delay(1000);

  // Check available networks
  Serial.println("4. Scanning networks:");
  sim_at_cmd("AT+COPS=?");
  delay(15000); // Wait longer as this command takes time

  // Check NET pin
  Serial.println("5. NET pin status:");
  check_network_status_pin();

  Serial.println("=== DIAGNOSTICS COMPLETE ===");
}

// Check network status
bool check_network_status()
{
  Serial.println("=== NETWORK STATUS ===");

  // Ensure module is awake
  wakeup_module();

  // Check network registration (detailed)
  Serial.println("1. Network registration:");
  sim_at_cmd("AT+CREG=2"); // Enable detailed reporting
  delay(500);
  sim_at_cmd("AT+CREG?");
  delay(1000);

  // Check signal quality
  Serial.println("2. Signal quality:");
  sim_at_cmd("AT+CSQ");
  delay(1000);

  // Check current operator
  Serial.println("3. Network operator:");
  sim_at_cmd("AT+COPS?");
  delay(1000);

  // Check radio mode
  Serial.println("4. Radio mode:");
  sim_at_cmd("AT+CFUN?");
  delay(1000);

  // Check NET pin hardware status
  Serial.println("5. NET pin status:");
  check_network_status_pin();

  return true;
}

// Attempt to reconnect to network with detailed steps
void reconnect_network()
{
  Serial.println("=== RECONNECTING TO NETWORK ===");

  // Ensure module is awake
  wakeup_module();

  // Step 1: Reset radio
  Serial.println("1. Turning off radio...");
  sim_at_cmd("AT+CFUN=0");
  delay(3000);

  Serial.println("2. Turning on radio...");
  sim_at_cmd("AT+CFUN=1");
  delay(5000);

  // Step 2: Set automatic registration mode
  Serial.println("3. Enabling automatic network registration...");
  sim_at_cmd("AT+CREG=2"); // Enable notification
  delay(1000);

  // Step 3: Select network automatically
  Serial.println("4. Selecting network automatically...");
  sim_at_cmd("AT+COPS=0");
  delay(15000); // Wait longer for registration

  // Step 4: Try manual network selection (Viettel)
  Serial.println("5. Trying to connect to Viettel...");
  sim_at_cmd("AT+COPS=1,2,\"45204\""); // Viettel MNC
  delay(10000);

  // Check result
  Serial.println("6. Checking result:");
  check_network_status();
}

// Non-blocking emergency SMS starter
void start_emergency_sms()
{
  if (!smsInProgress)
  {
    Serial.println("=== STARTING EMERGENCY SMS ===");
    wakeup_module();

    // Step 1: Set SMS text mode
    Serial.println("1. Setting SMS text mode...");
    sim_at_cmd("AT+CMGF=1"); // Text mode
    delay(1000);             // Wait for command to be processed

    // Step 2: Set phone number
    Serial.println("2. Setting recipient number...");
    String temp = "AT+CMGS=\"";
    temp += PHONE_NUMBER;
    temp += "\"";
    sim_at_cmd(temp);
    delay(2000); // Wait longer for prompt

    // Step 3: Send message content
    Serial.println("3. Sending message content...");
    Serial.println("Content: SOS DA MAT DIEN - HAY KIEM TRA THANG MAY TAZA");
    sim_at_cmd("SOS DA MAT DIEN - HAY KIEM TRA THANG MAY TAZA");
    delay(1000); // Wait before sending end character

    // Step 4: End message with Ctrl+Z
    Serial.println("4. Ending message...");
    sim_at_send(0x1A); // End message
    delay(500);        // Final delay

    smsInProgress = true;
    smsStartTime = millis();
    Serial.println("=== EMERGENCY SMS PROCESS STARTED ===");
  }
}

// Non-blocking emergency call starter with call limiting
void start_emergency_call()
{
  if (!callInProgress)
  {
    // Check if we're in cooldown period
    if (inCallCooldown)
    {
      if (millis() - callCooldownStartTime >= CALL_COOLDOWN_PERIOD)
      {
        // Cooldown period ended, reset for new batch
        inCallCooldown = false;
        callCount = 0;
        Serial.println("*** CALL COOLDOWN ENDED - READY FOR NEW BATCH ***");
      }
      else
      {
        // Still in cooldown, skip this call
        unsigned long remainingCooldown = CALL_COOLDOWN_PERIOD - (millis() - callCooldownStartTime);
        Serial.println("*** CALL COOLDOWN ACTIVE - " + String(remainingCooldown / 1000) + " seconds remaining ***");
        return;
      }
    }

    // Check if we've reached the call limit for current batch
    if (callCount >= MAX_CALLS_PER_BATCH)
    {
      // Start cooldown period
      inCallCooldown = true;
      callCooldownStartTime = millis();
      Serial.println("*** MAX CALLS REACHED (" + String(MAX_CALLS_PER_BATCH) + ") - STARTING 1 MINUTE COOLDOWN ***");
      return;
    }

    // Make the call
    wakeup_module();
    String temp = "ATD";
    temp += PHONE_NUMBER;
    temp += ";";
    sim_at_cmd(temp); // Make call

    callInProgress = true;
    callStartTime = millis();
    callCount++;

    Serial.println("*** EMERGENCY CALL " + String(callCount) + "/" + String(MAX_CALLS_PER_BATCH) + " INITIATED ***");

    // If this is the first call of a new batch, record batch start time
    if (callCount == 1)
    {
      callBatchStartTime = millis();
    }
  }
}

// Handle SMS process with timeout
void handle_sms_process()
{
  if (smsInProgress)
  {
    if (millis() - smsStartTime >= SMS_TIMEOUT)
    {
      smsInProgress = false;
      Serial.println("SMS process completed/timeout");

      // Start emergency call after SMS
      start_emergency_call();
    }
  }
}

// Handle call process with timeout
void handle_call_process()
{
  if (callInProgress)
  {
    if (millis() - callStartTime >= CALL_DURATION)
    {
      sim_at_cmd("ATH"); // Hang up
      callInProgress = false;
      Serial.println("Emergency call completed - hung up");
    }
  }
}

// Legacy blocking functions for manual testing
void sent_sms(String message)
{
  // Ensure module is awake
  wakeup_module();

  Serial.println("=== SENDING TEST SMS ===");
  Serial.println("1. Setting SMS text mode...");
  sim_at_cmd("AT+CMGF=1"); // Text mode
  delay(1000);             // Wait for command to be processed

  Serial.println("2. Setting recipient number...");
  String temp = "AT+CMGS=\"";
  temp += PHONE_NUMBER;
  temp += "\"";
  sim_at_cmd(temp);
  delay(2000); // Wait for prompt

  Serial.println("3. Sending message content...");
  Serial.println("Content: " + message);
  sim_at_cmd(message); // Message content
  delay(1000);         // Wait before ending

  Serial.println("4. Ending message...");
  // End message
  sim_at_send(0x1A);
  delay(500);

  Serial.println("=== TEST SMS COMPLETED ===");
}

void call()
{
  // Ensure module is awake
  wakeup_module();

  String temp = "ATD";
  temp += PHONE_NUMBER;
  temp += ";";
  sim_at_cmd(temp); // Make call

  // Non-blocking call - will be managed by handle_call_process()
  callInProgress = true;
  callStartTime = millis();
}

// Check call cooldown status (for monitoring)
void check_call_cooldown()
{
  static unsigned long lastCooldownReport = 0;

  if (inCallCooldown && millis() - lastCooldownReport >= 10000) // Report every 10 seconds
  {
    unsigned long remainingCooldown = CALL_COOLDOWN_PERIOD - (millis() - callCooldownStartTime);
    if (remainingCooldown > 0)
    {
      Serial.println(">>> Call cooldown: " + String(remainingCooldown / 1000) + " seconds remaining");
      lastCooldownReport = millis();
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\n\n\n\n-----------------------");
  Serial.println("A7683E System Starting!!!!");

  // Initialize timing variables
  lastSensorReadTime = millis();
  lastAtWaitTime = millis();

  // Initialize A7683E module
  init_a7683e_module();

  // Initialize Serial for SIM module
  simSerial.begin(MCU_SIM_BAUDRATE, SERIAL_8N1, MCU_SIM_RX_PIN, MCU_SIM_TX_PIN);

  // Initialize optocoupler sensor with pull-up resistor
  pinMode(optocoupler_Sensor, INPUT_PULLUP);

  // Read initial sensor state multiple times for debugging
  Serial.println("=== OPTOCOUPLER DEBUG INFO ===");
  for (int i = 0; i < 10; i++)
  {
    int reading = digitalRead(optocoupler_Sensor);
    Serial.println("Reading " + String(i + 1) + ": " + String(reading));
    delay(100);
  }

  lastSensorState = digitalRead(optocoupler_Sensor);
  Serial.println("Final initial sensor state: " + String(lastSensorState));
  Serial.println("Optocoupler Logic: 0=Power ON, 1=Power OFF/Lost");
  Serial.println("==============================");

  // Turn on GPIO pin 2 (like example)
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  Serial.println("=== POWER MONITORING SYSTEM ===");
  Serial.println("System initialized - no blocking delays!");

  // INITIALIZE MODULE SIM IMMEDIATELY (không đợi 5 giây)
  Serial.println("Initializing module communication IMMEDIATELY...");
  delay(2000);            // Chỉ đợi 2 giây để module ổn định
  sim_at_cmd("AT");       // Check AT command
  sim_at_cmd("ATI");      // Product information
  sim_at_cmd("AT+CPIN?"); // Check SIM slot
  sim_at_cmd("AT+CSQ");   // Check signal quality
  sim_at_cmd("AT+CIMI");  // Get IMSI
  moduleInitialized = true;
  Serial.println("Module communication initialized SUCCESSFULLY!");

  Serial.println("Monitoring power loss sensor...");
  Serial.println("Automatic SMS and call when power loss detected");
  Serial.println("Press '1' to test SMS");
  Serial.println("Press '2' to test call");
  Serial.println("Press '3' to check module status");
  Serial.println("Press '4' to test optocoupler diagnostics");
  Serial.println("===================================");
}

void loop()
{
  // MAIN FUNCTION: Read sensor and automatically notify on power loss (non-blocking)
  readSensor();

  // Handle ongoing SMS process
  handle_sms_process();

  // Handle ongoing call process
  handle_call_process();

  // Check call cooldown status
  check_call_cooldown();

  // Handle test commands from Serial Monitor
  if (Serial.available())
  {
    char command = Serial.read();

    switch (command)
    {
    case '1':
      if (!smsInProgress)
      {
        Serial.println("Testing SMS...");
        sent_sms("Test SMS from A7683E Power Monitor System");
        Serial.println("Test SMS initiated!");
      }
      else
      {
        Serial.println("SMS already in progress...");
      }
      break;

    case '2':
      if (!callInProgress)
      {
        Serial.println("Testing call...");
        call();
        Serial.println("Test call initiated!");
      }
      else
      {
        Serial.println("Call already in progress...");
      }
      break;

    case '3':
      Serial.println("=== Module Status ===");
      Serial.println("SMS in progress: " + String(smsInProgress ? "YES" : "NO"));
      Serial.println("Call in progress: " + String(callInProgress ? "YES" : "NO"));
      Serial.println("Last sensor state: " + String(lastSensorState));
      Serial.println("Call count in batch: " + String(callCount) + "/" + String(MAX_CALLS_PER_BATCH));
      Serial.println("In call cooldown: " + String(inCallCooldown ? "YES" : "NO"));
      if (inCallCooldown)
      {
        unsigned long remainingCooldown = CALL_COOLDOWN_PERIOD - (millis() - callCooldownStartTime);
        Serial.println("Cooldown remaining: " + String(remainingCooldown / 1000) + " seconds");
      }
      check_network_status_pin();
      break;

    case '4':
      Serial.println("=== OPTOCOUPLER DIAGNOSTICS ===");
      Serial.println("GPIO Pin: " + String(optocoupler_Sensor));

      // Test different configurations
      Serial.println("\n1. Testing INPUT mode:");
      pinMode(optocoupler_Sensor, INPUT);
      delay(10);
      for (int i = 0; i < 5; i++)
      {
        Serial.println("  Reading " + String(i + 1) + ": " + String(digitalRead(optocoupler_Sensor)));
        delay(200);
      }

      Serial.println("\n2. Testing INPUT_PULLUP mode:");
      pinMode(optocoupler_Sensor, INPUT_PULLUP);
      delay(10);
      for (int i = 0; i < 5; i++)
      {
        Serial.println("  Reading " + String(i + 1) + ": " + String(digitalRead(optocoupler_Sensor)));
        delay(200);
      }

      Serial.println("\n3. Testing INPUT_PULLDOWN mode:");
      pinMode(optocoupler_Sensor, INPUT_PULLDOWN);
      delay(10);
      for (int i = 0; i < 5; i++)
      {
        Serial.println("  Reading " + String(i + 1) + ": " + String(digitalRead(optocoupler_Sensor)));
        delay(200);
      }

      // Return to pull-up mode
      pinMode(optocoupler_Sensor, INPUT_PULLUP);
      Serial.println("\nReturned to INPUT_PULLUP mode");
      Serial.println("================================");
      break;

    default:
      // Forward other AT commands to SIM module if needed
      if (command != '\n' && command != '\r')
      {
        simSerial.write(command);
      }
      break;
    }
  }

  // Monitor NET pin status (keep to track network connection)
  monitor_net_status();

  // Handle SIM module responses (non-blocking)
  non_blocking_sim_at_wait();
}