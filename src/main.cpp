
/*
 * ESP32 với SIM A7683E Module - LTE Cat 1bis
 *
 * A7683E Features:
 * - LTE Cat 1bis với tốc độ download tối đa 10Mbps, upload 5Mbps
 * - Tương thích với AT commands như SIM800 series
 * - Hỗ trợ VoLTE, SMS, TCP/IP, HTTP/HTTPS, FTP, SSL
 * - Form factor LGA tương thích với SIM800C, SIM868, SIM7080G, A7682E
 *
 * Pin Configuration cho A7683E:
 * - VCC: 3.4V ~ 4.2V (Typ: 3.8V)
 * - GND: Ground
 * - TX (GPIO 17): ESP32 -> A7683E (UART transmit)
 * - RX (GPIO 16): ESP32 <- A7683E (UART receive)
 * - PEN (GPIO 23): Power Enable - Bật/tắt nguồn module
 * - NET (GPIO 5): Network Status - Chỉ báo trạng thái mạng
 * - DTR (GPIO 4): Data Terminal Ready - Sleep/Wake control
 *
 * Functions:
 * - PEN: HIGH = Bật nguồn module, LOW = Tắt nguồn
 * - NET: HIGH = Có mạng, LOW = Không có mạng
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

#define PHONE_NUMBER "+84374827208" // nhập số điện thoại

void sim_at_wait()
{
  delay(100);
  while (simSerial.available())
  {
    Serial.write(simSerial.read());
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

// Khoi tao A7683E Module
void init_a7683e_module()
{
  Serial.println("=== KHOI TAO A7683E MODULE ===");

  // 1. Cau hinh cac chan GPIO
  pinMode(MCU_SIM_PEN_PIN, OUTPUT);
  pinMode(MCU_SIM_NET_PIN, INPUT);
  pinMode(MCU_SIM_DTR_PIN, OUTPUT);

  // 2. Bat nguon module (PEN = HIGH)
  Serial.println("1. Bat nguon module...");
  digitalWrite(MCU_SIM_PEN_PIN, HIGH);
  delay(1000);

  // 3. Wake up module (DTR = LOW)
  Serial.println("2. Wake up module...");
  digitalWrite(MCU_SIM_DTR_PIN, LOW);
  delay(500);

  // 4. Cho module khoi dong
  Serial.println("3. Cho module khoi dong...");
  delay(3000);

  Serial.println("=== HOAN THANH KHOI TAO ===");
}

// Kiem tra trang thai NET pin
bool check_network_status_pin()
{
  bool net_status = digitalRead(MCU_SIM_NET_PIN);
  Serial.print("NET Pin Status: ");
  Serial.println(net_status ? "HIGH (Connected)" : "LOW (Disconnected)");
  return net_status;
}

// Sleep module A7683E
void sleep_module()
{
  Serial.println("Cho module vao sleep mode...");
  digitalWrite(MCU_SIM_DTR_PIN, HIGH);
  delay(100);
  Serial.println("Module da vao sleep mode (DTR=HIGH)");
}

// Wake up module A7683E
void wakeup_module()
{
  Serial.println("Danh thuc module...");
  digitalWrite(MCU_SIM_DTR_PIN, LOW);
  delay(100);
  Serial.println("Module da thuc day (DTR=LOW)");
}

// Power off module A7683E
void power_off_module()
{
  Serial.println("Tat nguon module...");
  digitalWrite(MCU_SIM_PEN_PIN, LOW);
  delay(100);
  Serial.println("Module da tat nguon (PEN=LOW)");
}

// Power on module A7683E
void power_on_module()
{
  Serial.println("Bat nguon module...");
  digitalWrite(MCU_SIM_PEN_PIN, HIGH);
  delay(1000);
  Serial.println("Module da bat nguon (PEN=HIGH)");
}

// Monitor NET pin trong thoi gian thuc
void monitor_net_status()
{
  static bool last_net_status = false;
  static unsigned long last_check = 0;

  if (millis() - last_check > 2000) // Check moi 2 giay
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

// Chan doan chi tiet SIM va mang
void diagnostic_sim()
{
  Serial.println("=== CHAN DOAN SIM ===");

  // Dam bao module thuc day
  wakeup_module();

  // Kiem tra SIM card
  Serial.println("1. Kiem tra SIM:");
  sim_at_cmd("AT+CPIN?");
  delay(1000);

  // Thong tin SIM
  Serial.println("2. IMSI:");
  sim_at_cmd("AT+CIMI");
  delay(1000);

  // Thong tin ICCID
  Serial.println("3. ICCID (so serial SIM):");
  sim_at_cmd("AT+CCID");
  delay(1000);

  // Kiem tra nha mang co san
  Serial.println("4. Quet nha mang:");
  sim_at_cmd("AT+COPS=?");
  delay(15000); // Doi lau vi lenh nay mat thoi gian

  // Kiem tra NET pin
  Serial.println("5. Trang thai NET pin:");
  check_network_status_pin();

  Serial.println("=== KET THUC CHAN DOAN ===");
}

// Kiem tra trang thai mang
bool check_network_status()
{
  Serial.println("=== TRANG THAI MANG ===");

  // Dam bao module thuc day
  wakeup_module();

  // Kiem tra dang ky mang (chi tiet)
  Serial.println("1. Dang ky mang:");
  sim_at_cmd("AT+CREG=2"); // Bat bao cao chi tiet
  delay(500);
  sim_at_cmd("AT+CREG?");
  delay(1000);

  // Kiem tra chat luong tin hieu
  Serial.println("2. Chat luong tin hieu:");
  sim_at_cmd("AT+CSQ");
  delay(1000);

  // Kiem tra nha mang hien tai
  Serial.println("3. Nha mang:");
  sim_at_cmd("AT+COPS?");
  delay(1000);

  // Kiem tra che do radio
  Serial.println("4. Che do radio:");
  sim_at_cmd("AT+CFUN?");
  delay(1000);

  // Kiem tra NET pin hardware
  Serial.println("5. Trang thai NET pin:");
  check_network_status_pin();

  return true;
}

// Thu ket noi lai mang voi cac buoc chi tiet
void reconnect_network()
{
  Serial.println("=== KET NOI LAI MANG ===");

  // Dam bao module thuc day
  wakeup_module();

  // Buoc 1: Reset radio
  Serial.println("1. Tat radio...");
  sim_at_cmd("AT+CFUN=0");
  delay(3000);

  Serial.println("2. Bat radio...");
  sim_at_cmd("AT+CFUN=1");
  delay(5000);

  // Buoc 2: Dat che do tu dong dang ky
  Serial.println("3. Bat tu dong dang ky mang...");
  sim_at_cmd("AT+CREG=2"); // Bat notification
  delay(1000);

  // Buoc 3: Chon nha mang tu dong
  Serial.println("4. Chon nha mang tu dong...");
  sim_at_cmd("AT+COPS=0");
  delay(15000); // Doi lau hon de dang ky

  // Buoc 4: Thu chon nha mang thu cong (Viettel)
  Serial.println("5. Thu ket noi Viettel...");
  sim_at_cmd("AT+COPS=1,2,\"45204\""); // Viettel MNC
  delay(10000);

  // Kiem tra ket qua
  Serial.println("6. Kiem tra ket qua:");
  check_network_status();
}

void sent_sms(String message)
{
  // Dam bao module thuc day
  wakeup_module();

  sim_at_cmd("AT+CMGF=1"); // Che do van ban
  String temp = "AT+CMGS=\"";
  temp += PHONE_NUMBER;
  temp += "\"";
  sim_at_cmd(temp);
  sim_at_cmd(message); // Noi dung tin nhan

  // Ket thuc tin nhan
  sim_at_send(0x1A);
}

void call()
{
  // Dam bao module thuc day
  wakeup_module();

  String temp = "ATD";
  temp += PHONE_NUMBER;
  temp += ";";
  sim_at_cmd(temp); // Gọi đi

  delay(20000); // Đợi 20 giây

  sim_at_cmd("ATH"); // Cúp máy
}

void setup()
{
  delay(20);
  Serial.begin(115200);
  Serial.println("\n\n\n\n-----------------------");
  Serial.println("He thong A7683E bat dau!!!!");

  // Khoi tao module A7683E
  init_a7683e_module();

  // Khoi tao Serial cho SIM module
  simSerial.begin(MCU_SIM_BAUDRATE, SERIAL_8N1, MCU_SIM_RX_PIN, MCU_SIM_TX_PIN);

  // Kiem tra lenh AT
  sim_at_cmd("AT");

  // Thong tin san pham - A7683E specific
  sim_at_cmd("ATI");

  // Kiem tra khe SIM
  sim_at_cmd("AT+CPIN?");

  // Kiem tra chat luong tin hieu
  sim_at_cmd("AT+CSQ");

  sim_at_cmd("AT+CIMI");

  // Bat GPIO pin 2 (giong example)
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  // Thu gui SMS test ngay
  Serial.println("Thu gui SMS test...");
  sent_sms("Test SMS tu ESP32 voi A7683E");

  // Doi 5 giay cho SMS
  delay(5000);

  // Hien thi huong dan su dung
  Serial.println("=== He thong dieu khien A7683E ===");
  Serial.println("Nhan '1' de gui SMS");
  Serial.println("Nhan '2' de thuc hien cuoc goi");
  Serial.println("Nhan '3' de kiem tra mang");
  Serial.println("Nhan '4' de ket noi lai mang");
  Serial.println("Nhan '5' de chan doan SIM");
  Serial.println("Nhan '6' de sleep module");
  Serial.println("Nhan '7' de wake up module");
  Serial.println("Nhan '8' de tat nguon module");
  Serial.println("Nhan '9' de bat nguon module");
  Serial.println("Nhan '0' de hien thi menu");
  Serial.println("===================================");
}

void loop()
{
  // Kiem tra lenh tu Serial Monitor
  if (Serial.available())
  {
    char command = Serial.read();

    switch (command)
    {
    case '1':
      Serial.println("Dang gui SMS...");
      sent_sms("Test SMS tu A7683E Module");
      Serial.println("SMS da duoc gui!");
      delay(2000); // Cho doi cho tin nhan duoc gui di
      break;

    case '2':
      Serial.println("Dang thuc hien cuoc goi...");
      call();
      Serial.println("Cuoc goi da ket thuc!");
      break;

    case '3':
      Serial.println("=== Kiem tra trang thai mang ===");
      check_network_status();
      break;

    case '4':
      Serial.println("=== Ket noi lai mang ===");
      reconnect_network();
      break;

    case '5':
      Serial.println("=== Chan doan SIM ===");
      diagnostic_sim();
      break;

    case '6':
      Serial.println("=== Cho module vao sleep mode ===");
      sleep_module();
      break;

    case '7':
      Serial.println("=== Danh thuc module ===");
      wakeup_module();
      break;

    case '8':
      Serial.println("=== Tat nguon module ===");
      power_off_module();
      break;

    case '9':
      Serial.println("=== Bat nguon module ===");
      power_on_module();
      break;

    case '0':
      Serial.println("=== Hien thi menu ===");
      Serial.println("=== He thong dieu khien A7683E ===");
      Serial.println("Nhan '1' de gui SMS");
      Serial.println("Nhan '2' de thuc hien cuoc goi");
      Serial.println("Nhan '3' de kiem tra mang");
      Serial.println("Nhan '4' de ket noi lai mang");
      Serial.println("Nhan '5' de chan doan SIM");
      Serial.println("Nhan '6' de sleep module");
      Serial.println("Nhan '7' de wake up module");
      Serial.println("Nhan '8' de tat nguon module");
      Serial.println("Nhan '9' de bat nguon module");
      Serial.println("Nhan '0' de hien thi menu");
      Serial.println("===================================");
      break;

    default:
      // Neu khong phai lenh da dinh nghia, chuyen tiep lenh AT den SIM module
      simSerial.write(command);
      break;
    }
  }

  // Monitor NET pin status
  monitor_net_status();

  // Hien thi phan hoi tu SIM module
  sim_at_wait();
}