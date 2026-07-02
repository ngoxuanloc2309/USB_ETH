# E-RA IoT PLATFORM — TÀI LIỆU KIẾN THỨC KỸ THUẬT TOÀN DIỆN
### (Dành cho Senior Embedded/IoT Engineer chuẩn bị tham gia dự án)

> Nguồn: https://e-ra-iot-wiki.gitbook.io/documentation (GitBook, đội ngũ EoH – Eye of Horus)
> Ngày tổng hợp: 02/07/2026

---

## 0. GHI CHÚ VỀ QUÁ TRÌNH THU THẬP TÀI LIỆU (minh bạch phạm vi)

Tài liệu E-Ra được tổ chức thành **10 chương La Mã (I → X)** + các nhóm phụ (Đăng ký/Quản lý tài khoản, Tính năng nâng cao, Dự án, Q&A). Thông qua file chỉ mục `llms.txt` do GitBook tự sinh, toàn bộ cây trang được liệt kê với khoảng **130+ trang con**. Trong phiên làm việc này tôi đã:

- Lấy được **toàn bộ sơ đồ mục lục (sitemap) đầy đủ** của site qua `llms.txt`.
- Đọc trực tiếp, sâu (fetch nội dung Markdown gốc) **~40 trang lõi**, bao phủ **100% các chương khái niệm, kiến trúc, cấu hình, API, giao thức, tính năng nâng cao** mà bạn yêu cầu (MQTT, REST API, Dashboard, Device, OTA/FOTA, Gateway, Plug & Play, Rule Engine/Automation, Webhook/Notification, Iframe, WiFi Config, Virtual Pin, Datastream, Widgets, Report, Map, Quản lý tài khoản...).
- Với nhóm trang **lặp lại theo từng loại phần cứng** (mỗi chip ESP32/ESP8266/STM32/RTL8720/Raspberry Pi/Asus Tinker Board đều có bộ trang con gần như giống hệt nhau về cấu trúc cho Arduino IDE / PlatformIO / Wokwi / MicroPython / Linux Terminal — ước tính ~40-50 trang) và nhóm **case-study "Dự án"/video demo/Q&A** (~20-30 trang), tôi đã xác nhận **khuôn mẫu (pattern) nội dung** qua các trang đại diện (ESP32, ESP8266) thay vì đọc từng trang riêng lẻ theo từng chip, vì nội dung chỉ khác nhau ở phần đấu nối chân/board — không ảnh hưởng đến mô hình kiến trúc hệ thống.

→ Nếu bạn cần đào sâu bất kỳ trang phần cứng cụ thể nào (ví dụ: "STM32 với PlatformIO", hoặc một "Dự án" case-study cụ thể), tôi có thể fetch chi tiết theo yêu cầu ở lượt sau.

---

## 1. TỔNG QUAN NỀN TẢNG

**E-Ra IoT Platform** là nền tảng IoT "Non-Code" (không cần lập trình phía cloud) do công ty **EoH (Eye of Horus)** — tiền thân là TeslaTeq (thành lập 2017, chuyên đo lường chất lượng nước công nghiệp) — phát triển và vận hành tại Việt Nam từ 2020.

**Giá trị cốt lõi:**
- Cung cấp Web Dashboard + Mobile App (iOS/Android) đồng nhất để giám sát/điều khiển thiết bị IoT.
- Cung cấp **bộ thư viện firmware (SDK) đa nền tảng** (Arduino IDE, PlatformIO, Wokwi, MicroPython, Linux) giúp lập trình viên nhúng kết nối nhanh phần cứng lên cloud.
- Công nghệ **"Plug & Play" đã được cấp bằng sáng chế (USPTO)** cho phép nhân bản cấu hình thiết bị cùng model chỉ trong < 5 phút bằng QR code, không cần cấu hình lại từ đầu trên Dashboard.
- Hỗ trợ tích hợp bên thứ 3 qua **3rd MQTT Gateway** và **Public REST API**.
- Tính đến thời điểm viết tài liệu: >9.650 trạm (Gateway), >10.807 cảm biến (Device), >32.255 thông số (Datastream) đang được giám sát liên tục.

---

## 2. KIẾN TRÚC TỔNG THỂ HỆ THỐNG

### 2.1 Mô hình phân cấp dữ liệu/tổ chức (Data & Org Hierarchy)

```
Organization (Tổ chức / tài khoản Admin quản lý)
   └── User (người dùng, phân quyền theo vai trò)
        └── Unit (Địa điểm — vd: "Trường học", "Nhà máy A")
             └── Sub-Unit (Khu vực/phòng ban trong Unit — vd: "Phòng bếp", "Khu vực giữ xe")
                  └── Gateway (Trạm/thiết bị trung tâm kết nối mạng — vd: "Trạm bơm")
                       └── Device (Thiết bị/ngoại vi vật lý gắn vào Gateway — vd: "Máy bơm", "Van xả")
                            └── Datastream (Luồng dữ liệu / kênh dữ liệu của Device — nhiệt độ, trạng thái relay...)
                                 └── Virtual Pin / GPIO Pin (điểm dữ liệu thực thi cụ thể, map vào biến firmware)
```

Ví dụ chính thức trong docs:
> Unit (Trường học) ⇒ Sub-Unit (Khu bếp) ⇒ Gateway (Trạm bơm) ⇒ Device 1 (Máy bơm) + Device 2 (Van xả)

Đây là **mô hình đa tenant phân cấp địa lý/tổ chức**, không phải mô hình "device-centric" đơn giản như nhiều nền tảng IoT khác (Blynk, ThingSpeak) — điểm này rất quan trọng khi thiết kế dự án có nhiều địa điểm/nhiều khách hàng.

### 2.2 Kiến trúc luồng dữ liệu (Data Flow Architecture)

```
[Cảm biến/Actuator vật lý]
        │ (I2C/UART/GPIO/Modbus RTU/Zigbee)
        ▼
[Firmware trên MCU/SBC — thư viện ERa SDK]
        │  (Wifi / 3G-4G / Ethernet / Lora / Bluetooth)
        │  MQTT (chuẩn), hoặc 3rd MQTT Gateway (bên thứ 3)
        ▼
[E-Ra Cloud Broker/Backend] ── xử lý Datastream, lưu trữ time-series
        │
        ├──► Web Dashboard (Widgets realtime, Report, Map)
        ├──► Mobile App (iOS/Android — cùng dữ liệu, đồng bộ realtime)
        ├──► Rule Engine (Scenario/Automation) → Action: Điều khiển thiết bị / Gửi Notification (Email/SMS) / Webhook ra hệ thống ngoài
        └──► Public REST API / MQTT Server (bên thứ 3 truy vấn/điều khiển từ ngoài)
```

### 2.3 Các thành phần chính (Core Components)

| Thành phần | Vai trò | Ghi chú kỹ thuật |
|---|---|---|
| **Gateway** | "Trạm" trung tâm — đại diện 1 thiết bị phần cứng có kết nối mạng, giữ **Authentication Token** riêng | Là đơn vị định danh kết nối MQTT/API chính; mỗi Gateway ràng buộc 1 loại phần cứng + firmware tương ứng |
| **Device** | Ngoại vi logic gắn vào Gateway (relay, sensor...) | Cấu hình GPIO/Virtual Pin, đơn vị đo, tỉ lệ scale |
| **Datastream** | Kênh dữ liệu (data channel) của 1 Device | Cấu hình: đơn vị, hệ số Scale, tần suất lưu, số chữ số thập phân, bộ lọc giá trị (filter) |
| **Virtual Pin (V-Pin)** | "Chân ảo" lập trình được — cầu nối giữa firmware và Dashboard, không phụ thuộc GPIO vật lý | Dùng để gửi/nhận giá trị tùy ý (không giới hạn bởi số chân MCU thực) |
| **Input/Output Pin** | Ánh xạ trực tiếp tới chân GPIO vật lý của board | Dùng cho điều khiển on/off cơ bản |
| **Widget** | Thành phần UI trên Dashboard (Button, Slide Range, Chart, Terminal Box...) | Kéo-thả, bind với Datastream/Virtual Pin |
| **Scenario / Automation (Rule Engine)** | Engine tự động hoá theo điều kiện (IF-THEN) | Có 2 dạng: Scenario (kịch bản định thời/điều kiện) và Automation (phản ứng theo sự kiện) |
| **3rd MQTT Gateway** | Cầu nối MQTT 2 chiều với hệ thống/thiết bị bên thứ 3 | Không cần dùng firmware ERa, chỉ cần tuân thủ topic/payload chuẩn |
| **Public E-Ra API** | REST API để CRUD dữ liệu, điều khiển thiết bị từ hệ thống ngoài | Dùng Authentication Token của Gateway/User |
| **Plug & Play** | Cơ chế nhân bản cấu hình bằng QR code | Cấp bằng sáng chế bởi EoH, áp dụng cho Gateway/Wifi Device/Modbus Device/Zigbee Device |
| **FOTA (Firmware Over-The-Air)** | Cập nhật firmware từ xa | Chỉ khả dụng cho Arduino Platform & Linux Library, **chưa hỗ trợ MicroPython** |
| **Local Control** | Điều khiển ngay tại biên (edge), không cần internet, dùng cho Automation | Giảm độ trễ, tăng độ tin cậy khi mất kết nối cloud |
| **iFrame** | Nhúng nội dung ngoài (web bản đồ, dashboard bên thứ 3...) vào Widget của E-Ra | Widget dạng "iFrame with config" |
| **WiFi Config** | Cấu hình Wifi cho thiết bị theo 2 cách: qua Webserver cục bộ trên thiết bị, hoặc qua Web Dashboard | Giải quyết bài toán cấp Wifi cho thiết bị không có màn hình |

---

## 3. LUỒNG HOẠT ĐỘNG TRIỂN KHAI DỰ ÁN (Project Onboarding Flow)

Theo tài liệu "III. Kiến thức Cơ bản", quy trình chuẩn để tích hợp 1 dự án E-Ra gồm các bước:

1. **Chuẩn bị phần cứng**: MCU/board có khả năng kết nối mạng (Wifi/Lora/Bluetooth/3G-4G/Ethernet), tương thích IDE phổ biến (Arduino IDE, PlatformIO, MicroPython), đủ tài nguyên xử lý FOTA + ghép nối cảm biến. E-Ra hỗ trợ mô phỏng trước bằng **Wokwi** (không cần phần cứng thật) để thử nghiệm nhanh.
2. **Đăng ký tài khoản**: mặc định có gói dùng thử miễn phí 6 tháng; nâng cấp gói tại `e-ra.io/vi/pricing`.
3. **Tạo Gateway online** trên Web Dashboard, đặt trong 1 **Unit** (địa điểm), chọn đúng loại phần cứng + mã nguồn mẫu tương ứng, hoàn tất kích hoạt Gateway.
4. **Tích hợp thư viện E-Ra vào firmware** theo IDE đã chọn (Arduino IDE / PlatformIO / Wokwi / MicroPython), nạp chương trình mẫu (có thể tùy biến thêm logic nghiệp vụ).
5. **Kiểm tra Gateway Online**: xác nhận thiết bị đã kết nối thành công lên cloud.
6. **Cấu hình Device**: khai báo ngoại vi (VD: GPIO27 → LED output, GPIO5 → Button input), định hình Datastream (thập phân, tỉ lệ, đơn vị).
7. **Thiết kế Dashboard**: kéo-thả Widget, bind Datastream/Virtual Pin.
8. **Thiết lập Automation/Scenario** (nếu cần tự động hoá) + Notification/Webhook.
9. **Nhân bản hàng loạt bằng Plug & Play** (nếu có nhiều thiết bị cùng loại) qua QR code.
10. **(Tuỳ chọn) Khai thác qua Public API / 3rd MQTT Gateway** để tích hợp hệ thống ngoài.

---

## 4. MQTT — CHI TIẾT KỸ THUẬT

E-Ra hỗ trợ **2 lớp giao tiếp MQTT**:

### 4.1 MQTT nội bộ (dùng bởi thư viện firmware ERa SDK)
Khi dùng firmware chính thức (Arduino/PlatformIO/MicroPython), thư viện tự động xử lý kết nối MQTT tới broker E-Ra bằng **Authentication Token** của Gateway — lập trình viên **không cần thao tác trực tiếp với topic/payload MQTT thô**, chỉ gọi API cấp cao (VD: `ERa.virtualWrite(V0, value)`).

### 4.2 3rd MQTT Gateway (tích hợp bên thứ 3, không dùng firmware ERa)
Đây là cơ chế cho phép **bất kỳ thiết bị/hệ thống nào hỗ trợ MQTT chuẩn** (không cần chạy thư viện ERa) kết nối trực tiếp vào E-Ra Cloud bằng cách tuân thủ đúng cấu trúc **topic** và **payload JSON**. Áp dụng khi:
- Thiết bị đã có sẵn firmware/hệ thống MQTT riêng (VD: PLC công nghiệp, Node-RED, hệ thống SCADA).
- Muốn kết nối nhanh không cần build lại firmware theo chuẩn E-Ra.

**Cấu trúc kết nối (Third-party MQTT Gateway)** gồm:
- **Broker/Host, Port**: thông tin kết nối MQTT server công khai của E-Ra (lấy tại mục **X. Public E-Ra API → MQTT Server**).
- **Authentication**: dùng Token (username/password kiểu MQTT) sinh ra từ Gateway đã tạo trên Dashboard.
- **Topic pattern**: theo cấu trúc chuẩn của E-Ra để publish dữ liệu lên Datastream tương ứng, và subscribe để nhận lệnh điều khiển xuống thiết bị.
- **Payload**: định dạng JSON chứa giá trị Datastream (theo Virtual Pin/Config ID tương ứng đã khai báo trên Dashboard).

**Use case điển hình**: kết nối PLC công nghiệp (S7-1200 qua Arduino/ESP32 làm gateway trung gian) hoặc cảm biến LoRa Gateway của bên thứ 3 lên E-Ra mà không cần viết lại toàn bộ firmware bằng ERa SDK.

> ⚠️ Lưu ý triển khai: khi dùng 3rd MQTT Gateway, việc mapping đúng Datastream ID/Virtual Pin với topic là điểm dễ sai nhất — nên tạo Gateway mẫu trên Dashboard trước, sao chép chính xác cấu hình topic hiển thị, rồi mới lập trình phía thiết bị.

### 4.3 MQTT Server (mục X — Public API)
Trang **"MQTT Server"** trong chương X liệt kê thông tin kết nối MQTT công khai (host/port/TLS) dùng chung cho cả firmware chính thức lẫn 3rd MQTT Gateway — đây là điểm vào giao thức duy nhất của toàn nền tảng.

---

## 5. REST API (X. Public E-Ra API)

E-Ra cung cấp **bộ Public REST API** để nhà phát triển bên ngoài (web app, mobile app riêng, hệ thống ERP/SCADA...) tương tác với dữ liệu mà không cần qua Dashboard:

- **Authentication**: dùng Token (Authentication Token của Gateway hoặc User, tạo/qu ản lý tại mục "Gateway & mã xác thực").
- **Chức năng chính** theo tài liệu:
  - Đọc dữ liệu Datastream (giá trị hiện tại, lịch sử — phục vụ Report).
  - Ghi/điều khiển giá trị Virtual Pin/Datastream (điều khiển thiết bị từ xa qua API).
  - Quản lý cấu hình Gateway/Device (CRUD ở mức cho phép).
- **Use case**: tích hợp E-Ra như 1 microservice dữ liệu trong hệ thống lớn hơn (dashboard nội bộ doanh nghiệp, app riêng cho khách hàng, kết nối với BI tool).

> Đây là kênh **song song** với MQTT: MQTT phù hợp cho luồng dữ liệu realtime tần suất cao (telemetry liên tục), còn REST API phù hợp cho thao tác rời rạc (query lịch sử, điều khiển theo yêu cầu, tích hợp hệ thống backend khác).

---

## 6. DASHBOARD (Web & Mobile)

### 6.1 Cấu trúc Dashboard
- **All Dashboard Units**: danh sách các Unit (địa điểm) — nơi bắt đầu điều hướng.
- **Tạo mới Unit (Địa điểm)**: định nghĩa 1 vị trí/dự án triển khai.
- **Tạo mới Sub-Unit & Device Display**: nhóm nhỏ hơn trong Unit + cách hiển thị thiết bị.
- **Edit Dashboard**: chỉnh sửa layout, kéo-thả Widget.
- **Quick Action / Quick View Icon**: icon truy cập nhanh trên Web và App để điều khiển/xem nhanh mà không cần vào sâu Dashboard.

### 6.2 Widget Library
Bộ Widget hỗ trợ kéo-thả gồm (theo mục lục VI):
| Nhóm | Widget |
|---|---|
| Điều khiển | Button, One Button, Three Button, State Grid, Number Action, Slide Range |
| Hiển thị | Text Box, Donut Chart, Status, Terminal Box |
| Nhúng | iFrame with config, Circle |

Mỗi Widget được bind trực tiếp với **Datastream/Virtual Pin** của 1 Device — khi giá trị thay đổi realtime qua MQTT, Widget tự cập nhật (WebSocket/push từ backend tới trình duyệt/app).

### 6.3 Report, Map, List/Overview/Healthy
- **Report**: xuất báo cáo lịch sử dữ liệu Datastream (biểu đồ, bảng) theo khoảng thời gian.
- **Map**: hiển thị vị trí địa lý các Gateway/Unit trên bản đồ (phù hợp bài toán giám sát phân tán như trạm quan trắc, xe/tài sản di động).
- **List / Overview / Healthy**: các view tổng hợp trạng thái nhiều thiết bị cùng lúc; **Overview** có tích hợp thiết lập **Notification qua Email/SMS** khi vượt ngưỡng cảnh báo (Alarm). *(Lưu ý: trang "Healthy" tại thời điểm crawl chưa có nội dung — có thể đang phát triển/documentation chưa cập nhật.)*

---

## 7. DEVICE — MÔ HÌNH THIẾT BỊ

"Device được xem là các ngoại vi (thiết bị) và liên kết đến Gateway" — Device **không tự kết nối mạng độc lập** mà luôn phụ thuộc vào 1 Gateway cha (Gateway là thứ giữ kết nối mạng + Token). Điều này có 2 dạng phổ biến:

1. **Device = chính MCU đó** (trường hợp đơn giản: 1 ESP32 vừa là Gateway vừa là Device, tự đọc/ghi GPIO của chính nó).
2. **Device = thiết bị con** kết nối qua Gateway trung gian bằng giao thức cục bộ: **Zigbee** (qua module Zigbee gắn với Gateway), **Modbus RTU** (qua cổng RS485 trên Gateway), **Bluetooth** (kết nối BLE cục bộ).

### 7.1 Zigbee Devices
Quy trình 2 giai đoạn: (1) Nạp code cho mạch Zigbee (module end-device), (2) Cấu hình thông số trên Dashboard để Gateway nhận diện và map Datastream. Zigbee Device là loại **duy nhất trong Plug & Play không cần QR code** — có thể add ngay.

### 7.2 Modbus Devices
Cấu hình Gateway đóng vai trò Modbus Master (RTU qua RS485), khai báo địa chỉ slave, thanh ghi (register) tương ứng ánh xạ vào Datastream. Phù hợp tích hợp thiết bị công nghiệp sẵn có (PLC, biến tần, đồng hồ đo điện) mà không cần thay thế phần cứng.

### 7.3 Bluetooth
Hỗ trợ kết nối cục bộ ngắn tầm (cấu hình ban đầu / giao tiếp cục bộ), thường dùng trong bước cấp Wifi ban đầu hoặc giám sát thiết bị gần Gateway.

---

## 8. OTA / FOTA (Firmware Over-The-Air)

**FOTA** cho phép cập nhật firmware từ xa qua Internet, quy trình 5 bước:

1. Build firmware từ source code thành file `*.bin`.
2. Truy cập tab **"Manage Firmware"** trong màn hình **"All Gateway"**.
3. Upload file `*.bin` lên hệ thống quản lý firmware của E-Ra.
4. Nhấn **"Shipping"** để đẩy firmware mới nhất xuống Gateway.
5. Refresh trang để kiểm tra trạng thái cập nhật.

**Lưu ý triển khai thực tế (rất quan trọng):**
- ⚠️ **FOTA hiện chỉ hỗ trợ Arduino Platform và Linux Library — CHƯA hỗ trợ MicroPython.** Nếu dự án dùng MicroPython (VD: Raspberry Pi Pico, ESP32 MicroPython), phải cập nhật firmware thủ công.
- Cần kiểm tra kỹ file `*.bin` trước khi Shipping — không có cơ chế rollback tự động rõ ràng trong tài liệu, rủi ro "brick" thiết bị nếu firmware lỗi.
- Tốc độ cập nhật phụ thuộc băng thông Internet tại vị trí Gateway/thiết bị — với triển khai công nghiệp/vùng sâu vùng xa cần tính toán thời gian chờ và có phương án fallback (kết nối 4G dự phòng).
- FOTA yêu cầu **phải có Gateway** (không áp dụng cho Device con thuần Zigbee/Modbus — logic hợp lý vì các Device đó không tự chạy firmware độc lập).

---

## 9. GATEWAY — TRUNG TÂM KẾT NỐI

Gateway là đơn vị "trạm" đại diện 1 phần cứng vật lý có kết nối mạng trực tiếp, đóng 3 vai trò:

1. **Định danh & xác thực**: mỗi Gateway có **Authentication Token** riêng — dùng cho cả kết nối MQTT (firmware ERa hoặc 3rd MQTT Gateway) và REST API.
2. **Quản lý firmware**: là nơi thực hiện FOTA.
3. **Cầu nối cho Device con**: các Device dùng Zigbee/Modbus/Bluetooth đều đi qua 1 Gateway cha để lên cloud.

**Quy trình tạo Gateway** ("All Gateways: Tạo và cấu hình Gateway"):
- Chọn loại phần cứng (ESP32, ESP8266, STM32, RTL8720, Raspberry Pi, Asus Tinker Board...).
- Hệ thống sinh mã nguồn mẫu + Token tương ứng.
- Đặt Gateway vào 1 Unit/Sub-Unit cụ thể.
- Kích hoạt (chờ Gateway Online lần đầu qua kết nối thực tế từ thiết bị).

---

## 10. PLUG AND PLAY — CÔNG NGHỆ NHÂN BẢN THIẾT BỊ

**Bản chất**: Công nghệ **đã được cấp bằng sáng chế bởi EoH (USPTO)**, cho phép nhân bản cấu hình 1 thiết bị "mẫu" đã hoàn chỉnh sang nhiều thiết bị **cùng model** chỉ trong < 5 phút, không cần cấu hình lại thủ công trên Dashboard cho từng thiết bị.

### Quy trình 3 bước:
1. **Tạo Gateway/Device mẫu hoàn chỉnh** trên Web (đầy đủ Widget, Datastream, cấu hình).
2. **Tạo mã QR code**: từ Gateway mẫu → chọn "Tạo mã QR code" → nhập tên "MyPrefix" (VD: `Sola`) → copy đoạn code sinh ra, **paste vào hàm `setup()`** của firmware thiết bị nhân bản.
   - Khi cấp nguồn, thiết bị mới sẽ phát Wifi AP cục bộ dạng `eoh.sola.xxxxxxxxx` để chờ người dùng quét QR kết nối.
   - Các chương trình mẫu có tên chứa **"PlugNPlay"** trong thư viện ERa được thiết kế sẵn để hỗ trợ cơ chế này.
3. **Add thiết bị bằng Mobile App**: quét QR → App tự động sao chép toàn bộ cấu hình từ mẫu sang thiết bị mới.

### 5 loại hỗ trợ Plug & Play:
| Loại | Cần QR code? |
|---|---|
| 1. Gateway | Có |
| 2. Wifi Device | Có |
| 3. Modbus Device | Có |
| 4. Zigbee Device | **Không** (add trực tiếp) |
| 5. Scan | *(đang phát triển tại thời điểm viết tài liệu)* |

**Lưu ý triển khai**:
- ⚠️ Tính năng **tạo mã QR không khả dụng với gói dùng thử (Free Trial)** — bắt buộc nâng cấp gói trả phí để dùng Plug & Play ở quy mô lớn (VD: lắp đặt 100 thiết bị đèn cùng loại).
- Chỉ áp dụng nhân bản trong phạm vi Gateway/Unit mà tài khoản đang thao tác **là chủ sở hữu**.
- Rất phù hợp cho **triển khai hàng loạt (mass deployment)**: nhà thông minh nhiều phòng cùng loại thiết bị, đèn đường đô thị thông minh, hệ thống tưới nông nghiệp nhiều điểm giống nhau.

---

## 11. RULE ENGINE / AUTOMATION (Smart Scenario & Automation)

Đây là "bộ não" tự động hoá của E-Ra, chia làm 2 khái niệm — cấu hình được **cả trên Web Dashboard lẫn Mobile App**:

### 11.1 Scenario (Kịch bản)
- Thiết lập hành động **theo lịch/thời gian hoặc điều kiện định trước**.
- Ví dụ chính thức: "Tự động bật đèn sau giờ ngủ trưa của học sinh" — tức trigger theo **thời gian (time-based)** hoặc theo **giá trị Datastream** đạt ngưỡng.

### 11.2 Automation
- Phản ứng **theo sự kiện tức thời (event-driven)**.
- Ví dụ chính thức: "Khi có người ra vào lớp thì gửi thông báo" — trigger theo **sự kiện cảm biến** (VD: cảm biến chuyển động/cửa) → Action tức thì.

### 11.3 Cấu trúc chung (IF – THEN)
Từ trang "Smart (Web)" và "Smart (Mobile App)", Rule Engine hoạt động theo mô hình:

```
CONDITION (Điều kiện)              ACTION (Hành động)
─────────────────────              ───────────────────
• Giá trị Datastream so sánh    →  • Điều khiển Device khác (bật/tắt/set value)
  (>, <, =, giữa khoảng...)     →  • Gửi Notification (Email/SMS/Push)
• Theo lịch (giờ/ngày/lặp lại)  →  • Gọi Webhook ra hệ thống ngoài (HTTP callback)
• Trạng thái Online/Offline     →  • Kích hoạt Scenario/Automation khác
  của Gateway/Device
```

### 11.4 Local Control (For Smart Automation)
Đây là tính năng **quan trọng về mặt kiến trúc**: cho phép **Automation chạy ngay tại biên (edge/local)**, không phụ thuộc hoàn toàn vào Cloud xử lý rule. Lợi ích:
- **Giảm độ trễ** phản ứng (không phải round-trip lên cloud rồi mới ra lệnh xuống).
- **Vẫn hoạt động khi mất kết nối Internet tạm thời** giữa Gateway và Cloud (rule đã đồng bộ sẵn xuống Gateway).
- Phù hợp bài toán an toàn/critical (VD: tự động tắt máy bơm khi cảm biến mực nước đầy — không thể chờ độ trễ cloud).

> ⚠️ Lưu ý triển khai: Local Control yêu cầu phần cứng Gateway đủ khả năng lưu trữ & thực thi rule cục bộ — không phải mọi board đều hỗ trợ như nhau; cần kiểm tra tài liệu tương thích theo từng dòng chip khi thiết kế bài toán an toàn.

---

## 12. WEBHOOK & NOTIFICATION

Dựa trên cấu trúc Action của Rule Engine (mục 11.3) và tính năng Alarm trong "Overview":

- **Notification**: kênh cảnh báo tích hợp sẵn gồm **Email** và **SMS**, cấu hình ngưỡng cảnh báo (Alarm) ngay tại view **Overview** của Dashboard — khi Datastream vượt ngưỡng, hệ thống tự gửi cảnh báo tới người quản lý.
- **Webhook**: là 1 dạng **Action** trong Scenario/Automation, cho phép gọi HTTP callback ra **hệ thống bên ngoài** khi điều kiện được kích hoạt — đây là cơ chế tích hợp E-Ra với hệ thống thứ 3 theo hướng **outbound (đẩy sự kiện ra ngoài)**, bổ sung cho hướng **inbound** của Public REST API/3rd MQTT Gateway (kéo dữ liệu vào E-Ra).

**Thiết kế tích hợp gợi ý cho dự án thực tế**:
```
E-Ra Rule Engine ──(Webhook HTTP POST)──► Hệ thống ERP/CRM/Chatbot nội bộ
E-Ra Rule Engine ──(Notification)───────► Email/SMS người vận hành
Hệ thống ngoài ───(Public REST API)─────► E-Ra (đọc/ghi dữ liệu)
Hệ thống ngoài ───(3rd MQTT Gateway)────► E-Ra (đẩy telemetry realtime)
```

---

## 13. IFRAME

Widget **"iFrame with config"** cho phép **nhúng nội dung web bên ngoài** (bản đồ tuỳ chỉnh, dashboard BI bên thứ 3, trang giám sát camera, trang trạng thái tuỳ biến...) trực tiếp vào layout Dashboard của E-Ra, thay vì chỉ giới hạn trong bộ Widget có sẵn.

**Ứng dụng thực tế**: nhúng Google Maps tuỳ biến, nhúng Grafana/Power BI panel, nhúng trang stream camera IP — biến Dashboard E-Ra thành 1 "cổng tổng hợp" (unified portal) thay vì chỉ hiển thị dữ liệu IoT thuần.

---

## 14. WIFI CONFIG

E-Ra hỗ trợ **2 phương thức cấp Wifi cho thiết bị không có màn hình/bàn phím**:

### 14.1 WiFi Config trên Webserver (cục bộ trên thiết bị)
- Thiết bị (firmware ERa) khi chưa có Wifi sẽ tự phát ra 1 Access Point (Wifi AP) cục bộ + host 1 webserver cấu hình nhỏ.
- Người dùng kết nối điện thoại/laptop vào AP đó → mở trình duyệt → nhập SSID/Password Wifi thật → thiết bị lưu cấu hình và kết nối lại vào mạng chính.
- Đây là pattern phổ biến giống "Wifi Manager" của ESP32/ESP8266 (captive portal), được ERa SDK đóng gói sẵn.

### 14.2 WiFi Config trên Web Dashboard
- Cấu hình/thay đổi Wifi **từ xa qua Dashboard** (khi thiết bị đã Online ít nhất 1 lần và có kết nối Internet để nhận lệnh đổi Wifi).
- Phù hợp khi cần đổi mạng Wifi cho thiết bị đã lắp đặt mà không muốn tiếp cận vật lý lại (VD: đổi router, đổi mật khẩu Wifi công ty).

> Lưu ý: phương án 14.2 có "con gà quả trứng" — thiết bị phải đang Online trên mạng cũ mới nhận được lệnh đổi mạng mới; nếu mạng cũ đã mất hoàn toàn phải quay về phương án 14.1 (cấu hình lại tại chỗ qua AP cục bộ).

---

## 15. CÁC TÍNH NĂNG NÂNG CAO KHÁC

| Tính năng | Mô tả | Trang nguồn |
|---|---|---|
| **Voice Control** | Điều khiển thiết bị bằng giọng nói, tích hợp qua cấu hình trên Web Dashboard | `tinh-nang-nang-cao/voice-control` |
| **Datastream Filter/Scale** | Bộ lọc giá trị nhiễu, hệ số quy đổi đơn vị, số thập phân hiển thị | `datastream` |
| **Report (Xuất báo cáo)** | Trích xuất dữ liệu lịch sử theo khoảng thời gian, dạng bảng/biểu đồ | `report` |
| **Map (Bản đồ)** | Định vị địa lý các trạm/Gateway | `map` |
| **E-Ra Affiliate Program (CTV)** | Chương trình tiếp thị liên kết — hoa hồng 10-20% theo số lượt giới thiệu thành công (21-50 lượt: 10%, 51-100: 15%, 101+: 20%), NGT nhận thêm 1 tháng sử dụng | `tiep-thi-lien-ket-cung-e-ra` |
| **Quản lý User/Org** | Phân quyền nhiều người dùng trong 1 tổ chức, vai trò Admin quản lý thành viên | `quan-ly-tai-khoan/quan-ly-user`, `quan-ly-org` |
| **Wokwi Simulation** | Mô phỏng phần cứng online, thử nghiệm firmware ERa mà không cần board thật | `ii.-bat-dau-nhanh/trai-nghiem-nhanh-e-ra-platform-voi-wokwi` |
| **Quick Template** | Mẫu dựng nhanh dự án (dashboard + firmware mẫu) | `ii.-bat-dau-nhanh/quick-template` |

---

## 16. VÍ DỤ SỬ DỤNG THỰC TẾ (từ tài liệu "Ứng dụng thực tế" & "Dự án")

Docs liệt kê nhiều nhóm ứng dụng mẫu (case study), phản ánh các ngành mục tiêu của E-Ra:

- **Giải pháp thành phố thông minh**: đèn chiếu sáng đường phố thông minh, giám sát chất lượng không khí ngoài trời.
- **Giám sát chất lượng nước** (mảng gốc/di sản từ TeslaTeq).
- **Giải pháp Wifi công cộng, quản lý tiện ích đô thị.**
- **Nhà thông minh**: đồ gia dụng thông minh, loa thông minh, đèn thông minh, hệ thống thông gió thông minh, giám sát chất lượng không khí trong nhà.
- **Công nghiệp/Nông nghiệp**: hệ thống tưới tự động, quản lý hàng tồn kho, giải pháp theo dõi nhân viên, giải pháp khẩn cấp, giám sát sức khoẻ từ xa.
- **Case study đào tạo (Workshop/Training)**:
  - Topic 1: Điều khiển đèn & giám sát nhiệt độ/độ ẩm.
  - Topic 2: Điều khiển & đo độ sáng LED qua Virtual Pin.
  - Topic 3: Nhà thông minh với bộ kit Ohstem.
  - Topic 4: Nông trại thông minh.
  - Ứng dụng Arduino/ESP32 kết nối PLC công nghiệp Siemens **S7-1200** lên E-Ra (minh hoạ rõ use case tích hợp công nghiệp qua Modbus/3rd MQTT Gateway).

### Ví dụ thực hành chi tiết trong mục IX (Thực hành):
- Điều khiển bật/tắt LED với ESP32 (Output Pin cơ bản).
- Module 4G SIMCOM A7680 với ESP32 (tính năng SMS Alarm — minh hoạ Notification qua SMS ở tầng thiết bị, không chỉ qua Cloud).
- CC2530 với ESP32 (tích hợp thiết bị Zigbee).
- Kết nối thiết bị Modbus với E-Ra qua ASUS Tinker Board.
- GPS Tracking; Theo dõi nhiệt độ & độ ẩm phòng với ESP32.
- Điều khiển LED & đọc trạng thái nút nhấn qua PLC-Modbus RTU.

---

## 17. BEST PRACTICES (đúc kết từ tài liệu + góc nhìn kỹ sư triển khai)

1. **Luôn tạo Gateway "mẫu" hoàn chỉnh trước khi nhân bản** — thiết kế đúng Widget/Datastream/Scale ngay từ đầu, vì Plug & Play chỉ copy chính xác những gì đã cấu hình; sai từ mẫu sẽ nhân sai hàng loạt.
2. **Tách rõ vai trò Gateway vs Device**: đừng nhầm lẫn — chỉ Gateway giữ Token/kết nối mạng; nếu dự án có nhiều cảm biến nhỏ cùng khu vực, nên gom vào 1 Gateway trung tâm qua Modbus/Zigbee thay vì mỗi cảm biến 1 Gateway riêng (tối ưu chi phí kết nối + đơn giản hoá bảo trì mạng).
3. **Dùng Virtual Pin thay vì Input/Output Pin vật lý khi có thể** — vì Virtual Pin không bị giới hạn bởi số chân GPIO thực, linh hoạt hơn khi mở rộng số lượng Datastream.
4. **Kiểm thử trên Wokwi trước khi mua/triển khai phần cứng thật** — giảm rủi ro và chi phí ở giai đoạn PoC (Proof of Concept).
5. **Với bài toán an toàn/real-time-critical, ưu tiên Local Control** thay vì Automation thuần cloud — tránh phụ thuộc độ trễ mạng/uptime cloud cho các hành động cấp thiết (an toàn cháy nổ, ngập nước...).
6. **Chuẩn hoá quy trình FOTA trong CI/CD nội bộ**: luôn test file `.bin` trên 1 thiết bị staging trước khi "Shipping" hàng loạt — tài liệu không đề cập cơ chế rollback tự động, nên rủi ro cao nếu update lỗi trên diện rộng.
7. **Khi tích hợp 3rd MQTT Gateway**, luôn đối chiếu chính xác Topic/Payload sinh ra từ Gateway mẫu trên Dashboard — sai lệch nhỏ về ID sẽ khiến dữ liệu "biến mất" (không lỗi rõ ràng, chỉ không thấy dữ liệu lên Datastream).
8. **Phân quyền User/Org rõ ràng ngay từ đầu dự án** nhiều người dùng — tránh tình trạng nhiều kỹ sư cùng sửa 1 Gateway gây xung đột cấu hình.
9. **Cân nhắc gói trả phí sớm nếu dự án cần Plug & Play ở quy mô lớn** — tính năng QR code bị khoá ở gói Free Trial, dễ gây trễ tiến độ nếu phát hiện muộn.
10. **Đặt tên Unit/Sub-Unit/Gateway/Device theo quy ước rõ ràng** (namespace) ngay từ đầu — vì mô hình phân cấp sâu (Org → User → Unit → Sub-Unit → Gateway → Device → Datastream), dự án quy mô vừa/lớn rất dễ rối nếu không có convention đặt tên nhất quán.

---

## 18. LƯU Ý KHI TRIỂN KHAI THỰC TẾ (Rủi ro & Giới hạn cần biết trước)

| Hạng mục | Lưu ý / Giới hạn |
|---|---|
| **FOTA** | Không hỗ trợ MicroPython; không có rollback tự động rõ ràng; phụ thuộc băng thông Internet tại hiện trường |
| **Plug & Play – QR code** | Không khả dụng ở gói dùng thử (Free Trial); chỉ chủ sở hữu Gateway/Unit mới thao tác được |
| **Loại "Scan" trong Plug & Play** | Tại thời điểm tài liệu được viết, tính năng này còn ghi "Đang cập nhật" — chưa hoàn thiện, không nên phụ thuộc vào nó cho roadmap gần |
| **Trang "Healthy" (Dashboard)** | Nội dung tài liệu trống tại thời điểm crawl — tính năng có thể đang phát triển, cần xác nhận trực tiếp trên sản phẩm thay vì chỉ dựa vào docs |
| **Chi phí / Subscription** | Không có hoàn tiền (refund) cho thời gian còn lại của gói đã đăng ký khi huỷ hoặc đổi gói — cần tính toán kỹ trước khi nâng cấp |
| **Thanh toán** | Trong giai đoạn Soft Launch, chỉ chấp nhận chuyển khoản ngân hàng theo thông tin được cung cấp khi liên hệ Sales — không tự động qua thẻ tín dụng (cần xác nhận lại tình trạng hiện tại vì đây có thể đã thay đổi) |
| **Yêu cầu kiến thức người dùng** | E-Ra **không phải no-code hoàn toàn ở tầng thiết bị** — người dùng vẫn cần biết lập trình phần cứng (Arduino/C++/MicroPython) để nạp firmware; "Non-Code" chỉ áp dụng cho tầng Cloud/Dashboard |
| **Zigbee** | Cần mạch Zigbee riêng (VD: CC2530) đóng vai trò end-device, Gateway chính (ESP32...) đóng vai trò điều phối — kiến trúc 2 lớp phần cứng, không phải Zigbee tích hợp sẵn trong mọi board |
| **Modbus** | Yêu cầu Gateway có cổng RS485 vật lý (hoặc module chuyển đổi) — cần xác nhận board đã chọn có hỗ trợ trước khi thiết kế hardware BOM |
| **3rd MQTT Gateway** | Không có cơ chế kiểm tra/gợi ý lỗi rõ ràng khi Topic/Payload sai — nên có logging riêng phía thiết bị bên thứ 3 để debug độc lập với Dashboard E-Ra |
| **Đa ngôn ngữ tài liệu** | Phần lớn nội dung bằng tiếng Việt, một số trang song ngữ (EN/VI không đồng bộ 100%) — khi làm việc với đối tác quốc tế cần tự dịch/chuẩn hoá thuật ngữ |

---

## 19. TÓM TẮT SƠ ĐỒ KIẾN TRÚC (dạng khối cho kỹ sư mới)

```
┌─────────────────────────────────────────────────────────────────┐
│                        E-RA CLOUD PLATFORM                       │
│  ┌───────────┐   ┌───────────┐   ┌──────────────┐   ┌─────────┐  │
│  │ Rule       │   │ Datastream │   │ Auth /       │   │ Report/ │  │
│  │ Engine     │   │ Storage    │   │ Token Mgmt   │   │ Report  │  │
│  │ (Scenario/ │   │ (Time-     │   │ (Gateway/    │   │ Engine  │  │
│  │ Automation)│   │  series)   │   │  User Token) │   │         │  │
│  └─────┬─────┘   └─────┬─────┘   └──────┬───────┘   └────┬────┘  │
│        │  MQTT Broker  │  REST API Layer │  Webhook Dispatcher   │
└────────┼───────────────┼─────────────────┼───────────────┼───────┘
         │               │                 │               │
   ┌─────▼─────┐   ┌─────▼──────┐   ┌──────▼──────┐  ┌─────▼─────┐
   │ ERa SDK    │   │ 3rd MQTT   │   │ Hệ thống    │  │ Email/SMS │
   │ Firmware   │   │ Gateway    │   │ ngoài (ERP, │  │ Người dùng│
   │ (ESP32/    │   │ (PLC/SCADA)│   │ CRM, BI...) │  │           │
   │ STM32/RPi..)│  └────────────┘   └─────────────┘  └───────────┘
   └─────┬──────┘
         │ GPIO/UART/I2C/Modbus/Zigbee/BLE
   ┌─────▼──────────────────────┐
   │ Cảm biến / Actuator vật lý  │
   └─────────────────────────────┘

            ▲                                    ▲
            │           Realtime sync            │
   ┌────────┴─────────┐              ┌───────────┴────────┐
   │  Web Dashboard    │◄────────────►│   Mobile App        │
   │  (Widgets, Map,   │              │   (iOS/Android)      │
   │   Report, Rule    │              │   (đồng bộ tính năng │
   │   Engine UI)       │              │    tương đương Web)  │
   └────────────────────┘              └──────────────────────┘
```

---

## 20. PHỤ LỤC — MỤC LỤC TÀI LIỆU GỐC (theo llms.txt, đã nhóm lại)

- **Giới thiệu E-Ra IoT Platform**
- **Đăng ký, Đăng nhập & Quản lý tài khoản** (User, Org, Admin, HSSV)
- **E-Ra Affiliate Program**
- **Tính năng nâng cao**: Voice Control, Plug & Play (+ Add Gateway/Wifi/Zigbee/Modbus Device), FOTA, Smart Scenario/Automation (Web + App), 3rd MQTT Gateway, WiFi Config (Webserver + Dashboard), iFrame, Local Control
- **I. Phần cứng hỗ trợ**
- **II. Bắt đầu nhanh**: trải nghiệm nhanh với ESP32/Wokwi, Quick Template
- **III. Kiến thức cơ bản**: Web Dashboard, Gateway, Scenario/Automation, App Mobile (giải thích thuật ngữ)
- **IV. Kết nối thiết bị phần cứng (Hardware)**: ESP32, STM32, Raspberry Pi, ESP8266, RTL8720, Asus Tinker Board
- **V. Kết nối thiết bị với E-Ra (Firmware)**: hướng dẫn theo từng IDE (Arduino IDE, PlatformIO, Wokwi, MicroPython, Linux Terminal) × từng chip
- **VI. Cấu hình và sử dụng Web Dashboard**: All Dashboard Units, Tạo Unit/Sub-Unit, Edit Dashboard, All Gateways (Token, I/O Pin, Virtual Pin, Zigbee, Modbus, Datastream, Bluetooth), Widgets (đầy đủ loại), Report, Map, Quick Action, List/Overview/Healthy
- **VII. E-Ra Mobile App**
- **IX. Thực hành**: ví dụ theo từng chip (ESP32, Raspberry Pi, Modbus RTU, Asus Tinker Board) + các ứng dụng thực tế (GPS Tracking, nhiệt độ/độ ẩm, PLC-Modbus)
- **X. Public E-Ra API**: API, MQTT Server
- **Dự án** (case study theo ngành: thành phố thông minh, nhà thông minh, công nghiệp, nông nghiệp, y tế...)
- **Q&A**, **Youtube Channel**

---

*Hết tài liệu tổng hợp. Tài liệu này được xây dựng để dùng làm tài liệu onboarding kỹ thuật cho kỹ sư mới tham gia dự án tích hợp E-Ra IoT Platform — nên đối chiếu lại với sản phẩm thực tế (UI hiện hành) trước khi triển khai production, vì một số chi tiết UI/tính năng có thể đã thay đổi kể từ thời điểm tài liệu gốc được cập nhật lần cuối (27/02/25 – 20/12/24 tuỳ trang).*