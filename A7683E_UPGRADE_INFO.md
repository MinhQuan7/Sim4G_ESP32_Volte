# A7683E Module Upgrade - Thông tin nâng cấp

## Thay đổi từ A7680 sang A7683E

### 1. Thông tin Module A7683E

- **Công nghệ**: LTE Cat 1bis
- **Tốc độ**: Download 10Mbps, Upload 5Mbps
- **Tương thích**: AT commands SIM800 series
- **Điện áp hoạt động**: 3.4V ~ 4.2V (Typical: 3.8V)
- **Form factor**: LGA tương thích SIM800C, SIM868, SIM7080G, A7682E

### 2. Pin Configuration mới

| Chân    | GPIO   | Chức năng               | Mô tả                            |
| ------- | ------ | ----------------------- | -------------------------------- |
| VCC     | -      | Power Supply            | 3.4V ~ 4.2V                      |
| GND     | -      | Ground                  | Mass chung                       |
| TX      | 17     | UART Transmit           | ESP32 -> A7683E                  |
| RX      | 16     | UART Receive            | ESP32 <- A7683E                  |
| **PEN** | **23** | **Power Enable**        | **HIGH=Bật, LOW=Tắt nguồn**      |
| **NET** | **5**  | **Network Status**      | **HIGH=Có mạng, LOW=Không mạng** |
| **DTR** | **4**  | **Data Terminal Ready** | **LOW=Wake, HIGH=Sleep**         |

### 3. Tính năng mới được thêm

#### A. Power Management

- `init_a7683e_module()`: Khởi tạo module với đúng thứ tự
- `power_on_module()`: Bật nguồn module (PEN=HIGH)
- `power_off_module()`: Tắt nguồn module (PEN=LOW)

#### B. Sleep/Wake Control

- `sleep_module()`: Cho module vào sleep mode (DTR=HIGH)
- `wakeup_module()`: Đánh thức module (DTR=LOW)
- Tự động wake up trước khi thực hiện lệnh AT

#### C. Network Status Monitoring

- `check_network_status_pin()`: Đọc trạng thái NET pin
- `monitor_net_status()`: Monitor liên tục trong loop()
- Tự động thông báo khi trạng thái mạng thay đổi

### 4. Menu điều khiển mới

| Phím  | Chức năng                |
| ----- | ------------------------ |
| 1     | Gửi SMS                  |
| 2     | Thực hiện cuộc gọi       |
| 3     | Kiểm tra trạng thái mạng |
| 4     | Kết nối lại mạng         |
| 5     | Chẩn đoán SIM            |
| **6** | **Sleep module**         |
| **7** | **Wake up module**       |
| **8** | **Tắt nguồn module**     |
| **9** | **Bật nguồn module**     |
| **0** | **Hiển thị menu**        |

### 5. Cải tiến trong code

#### A. Proper Power Sequence

```cpp
// Khởi tạo đúng thứ tự
1. Cấu hình GPIO pins
2. Bật nguồn (PEN=HIGH)
3. Wake up (DTR=LOW)
4. Chờ module khởi động
```

#### B. Enhanced Diagnostic

```cpp
- Kiểm tra SIM card
- Kiểm tra IMSI/ICCID
- Quét nhà mạng
- Kiểm tra NET pin hardware
```

#### C. Auto Wake Management

- Tự động wake up trước SMS/Call
- Tự động wake up trước AT commands
- Monitor trạng thái mạng liên tục

### 6. Lợi ích của A7683E

1. **Tốc độ cao hơn**: LTE Cat 1bis vs 2G/3G
2. **Tiết kiệm điện**: Sleep/Wake control tốt hơn
3. **Giám sát mạng**: NET pin hardware feedback
4. **Tương thích tốt**: AT commands giống SIM800
5. **Tính năng đầy đủ**: VoLTE, SSL, GPS (nếu có)

### 7. Test và Validation

#### Kiểm tra cơ bản:

1. Upload code và mở Serial Monitor
2. Quan sát quá trình khởi tạo
3. Test gửi SMS (phím 1)
4. Test cuộc gọi (phím 2)
5. Test sleep/wake (phím 6/7)
6. Test power on/off (phím 8/9)

#### Monitor NET pin:

- Quan sát thông báo "NET Status changed"
- NET=HIGH khi có mạng
- NET=LOW khi mất mạng

### 8. Troubleshooting

**Nếu module không khởi động:**

- Kiểm tra điện áp 3.4-4.2V
- Kiểm tra kết nối PEN pin (GPIO 23)
- Kiểm tra kết nối DTR pin (GPIO 4)

**Nếu không kết nối mạng:**

- Kiểm tra NET pin (GPIO 5)
- Chạy diagnostic (phím 5)
- Chạy reconnect (phím 4)

**Nếu không gửi được SMS:**

- Đảm bảo có mạng (NET=HIGH)
- Kiểm tra SIM card (phím 5)
- Wake up module trước (phím 7)

---

_Code được nâng cấp từ A7680 sang A7683E với đầy đủ tính năng power management và network monitoring._
