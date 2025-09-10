
#include <HardwareSerial.h>

#define simSerial Serial2
#define MCU_SIM_BAUDRATE 115200
#define MCU_SIM_TX_PIN 17
#define MCU_SIM_RX_PIN 16
#define MCU_SIM_EN_PIN 15

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

// Chan doan chi tiet SIM va mang
void diagnostic_sim()
{
  Serial.println("=== CHAN DOAN SIM ===");

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

  Serial.println("=== KET THUC CHAN DOAN ===");
}

// Kiem tra trang thai mang
bool check_network_status()
{
  Serial.println("=== TRANG THAI MANG ===");

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

  return true;
}

// Thu ket noi lai mang voi cac buoc chi tiet
void reconnect_network()
{
  Serial.println("=== KET NOI LAI MANG ===");

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
  String temp = "ATD";
  temp += PHONE_NUMBER;
  temp += ";";
  sim_at_cmd(temp); // Gọi đi

  delay(20000); // Đợi 20 giây

  sim_at_cmd("ATH"); // Cúp máy
}

void setup()
{
  /*  Bật nguồn mô-đun SIM  */
  pinMode(MCU_SIM_EN_PIN, OUTPUT);
  digitalWrite(MCU_SIM_EN_PIN, LOW);

  delay(20);
  Serial.begin(115200);
  Serial.println("\n\n\n\n-----------------------\nHe thong bat dau!!!!");

  // Doi 8 giay de mo-dun SIM khoi dong
  delay(8000);
  simSerial.begin(MCU_SIM_BAUDRATE, SERIAL_8N1, MCU_SIM_RX_PIN, MCU_SIM_TX_PIN);

  // Kiem tra lenh AT
  sim_at_cmd("AT");

  // Thong tin san pham
  sim_at_cmd("ATI");

  // Kiem tra khe SIM
  sim_at_cmd("AT+CPIN?");

  // Kiem tra chat luong tin hieu
  sim_at_cmd("AT+CSQ");

  sim_at_cmd("AT+CIMI");

  // Bat GPIO pin 2 (giong example)
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  // Thu gui SMS test ngay (giong example)
  Serial.println("Thu gui SMS test...");
  sent_sms("Test SMS tu ESP32");

  // Doi 5 giay cho SMS
  delay(5000);

  // Hien thi huong dan su dung
  Serial.println("=== He thong dieu khien SIM ===");
  Serial.println("Nhan '1' de gui SMS");
  Serial.println("Nhan '2' de thuc hien cuoc goi");
  Serial.println("Nhan '3' de kiem tra mang");
  Serial.println("Nhan '4' de ket noi lai mang");
  Serial.println("Nhan '5' de chan doan SIM");
  Serial.println("===============================");
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
      sent_sms("Em Quan ne Anh Thao");
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

    default:
      // Neu khong phai lenh da dinh nghia, chuyen tiep lenh AT den SIM module
      simSerial.write(command);
      break;
    }
  }

  // Hien thi phan hoi tu SIM module
  sim_at_wait();
}