# Ôn tập Lập trình Mạng

## Chương 1: Ôn tập Mạng máy tính

### 1. Khái niệm Cơ bản về mạng máy tính
- **Định nghĩa**: Tập hợp các máy tính (máy trạm, máy chủ, bộ định tuyến) kết nối với nhau để chia sẻ tài nguyên và dữ liệu.
- **Hình dạng Mạng**: Các bố cục phổ biến bao gồm trục (Bus), vòng (Ring), và sao (Star).
- **Giao thức**: Các quy tắc tiêu chuẩn để thiết lập giao tiếp giữa các thiết bị.

### 2. Các mô hình OSI và TCP/IP
- **Kiến trúc phân tầng**: Phân chia các chức năng mạng để tối ưu hóa (dễ bảo trì và nâng cấp).
- **Mô hình OSI**: 7 tầng – Vật lý, Liên kết Dữ liệu, Mạng, Giao vận, Phiên, Trình diễn, và Ứng dụng.
- **Mô hình TCP/IP**: 4 tầng – Ứng dụng, Giao vận (TCP/UDP), Internet (IP), và Giao diện Mạng.

### 3. Giao thức IP và định địa chỉ
- **Chức năng của IP**: Chọn đường đi (Routing) và Chuyển tiếp (Forwarding) dữ liệu.
- **Địa chỉ IPv4**: 32-bit duy nhất, chia thành ID Mạng và ID Máy trạm.
- **Các Lớp Địa chỉ**: A, B, C, D, và E với CIDR (Classless Inter-Domain Routing).
- **Mặt Nạ Mạng**: Tách phần mạng và phần máy trạm.

### 4. Tầng Giao vận
- **Vai trò**: Hỗ trợ giao tiếp giữa các ứng dụng trên các máy khác nhau.
- **UDP**: Không cần liên kết, nhanh chóng, phù hợp cho ứng dụng yêu cầu thời gian thực.
- **TCP**: Tin cậy, hướng liên kết, có kiểm soát luồng, kiểm soát tắc nghẽn và kiểm tra lỗi.
- **Bắt tay ba bước**: Thiết lập kết nối TCP bằng SYN, SYN-ACK, ACK.

### 5. Tên miền (DNS)
- **Tên miền**: Địa chỉ dễ nhớ, ánh xạ với địa chỉ IP.
- **Phân giải Tên**: Chuyển đổi tên miền sang địa chỉ IP qua DNS.

### 6. Tầng ứng dụng
- **Mô hình Khách-Chủ**: Client yêu cầu dịch vụ từ Server (ví dụ, Web, Mail).
- **Mô hình Điểm-Điểm (P2P)**: Giao tiếp trực tiếp giữa các nút.
- **Mô hình Lai**: Kết hợp P2P và Khách-Chủ (ví dụ, Skype).

### 7. Giao tiếp giữa các tiến trình và Sockets
- **Sockets**: Điểm cuối để gửi/nhận dữ liệu, xác định bởi IP và số cổng.
- **Giao thức Tầng Ứng dụng**: Xác định các loại thông điệp và quy tắc giao tiếp.

## Chương 3: Socket APIs

### 1. Khái niệm Cơ bản về Socket
- **Socket Pair**: Bộ bốn yếu tố xác định điểm kết nối giữa hai đầu.
- **Cổng TCP**: Cho phép nhiều dịch vụ qua các cổng đặc biệt.

### 2. Kích thước bộ đệm và giới hạn
- **MTU**: Đơn vị truyền tối đa, phân mảnh nếu vượt quá.
- **MSS**: Kích thước phân đoạn tối đa.

### 3. Sockets API
- **Cấu trúc Địa chỉ Socket (IPv4)**: `sockaddr_in`.
- **Tham số Giá trị - Kết quả**: Tham chiếu từ quá trình đến nhân hệ điều hành.
  
### 4. Chức năng chuyển đổi địa chỉ
- **inet_aton()**: Chuyển đổi IPv4 thành nhị phân.
- **inet_ntoa()**: Chuyển đổi nhị phân IPv4 thành chuỗi.
- **inet_pton()**: Chuyển đổi địa chỉ IPv4/IPv6 từ chuỗi sang nhị phân.
- **inet_ntop()**: Chuyển đổi địa chỉ IPv4/IPv6 từ nhị phân sang chuỗi.

### 5. Hàm Đọc và Ghi
- **read()**: Đọc dữ liệu từ socket.
- **write()**: Gửi dữ liệu đến socket.

## Chương 4: TCP sockets

### Các hàm chính
- **socket()**: Tạo socket.
- **connect()**: Thiết lập kết nối giữa client và server.
- **bind()**: Liên kết socket với một địa chỉ IP và cổng.
- **listen()**: Chuyển socket sang trạng thái chờ kết nối.
- **accept()**: Server chấp nhận kết nối từ client.
- **fork()**: Tạo tiến trình con để xử lý client.
- **close()**: Đóng socket, kết thúc kết nối.
- **getsockname() và getpeername()**: Lấy địa chỉ socket cục bộ và đối tác kết nối.

## Chương 5: I/O Multiplexing

### 1. Giới thiệu về I/O Multiplexing
- **Vấn đề**: Xử lý đồng thời nhiều nguồn dữ liệu.
- **Giải pháp**: Nhận thông báo từ hệ điều hành khi có điều kiện I/O sẵn sàng.

### 2. Các Mô hình I/O
- **Blocking I/O**: Chờ I/O hoàn thành.
- **Non-blocking I/O**: Không chờ hoàn thành.
- **I/O Multiplexing**: Dùng `select()` và `poll()` để chờ sự kiện từ nhiều file descriptors.
- **Signal-driven I/O**: Dùng tín hiệu để thông báo.
- **Asynchronous I/O**: Dùng `aio_*` theo chuẩn POSIX.

### 3. Hàm select()
- **select()**: Chờ nhiều sự kiện từ nhiều file descriptors.
  
### 4. Hàm pselect()
- **pselect()**: Chờ sự kiện với độ phân giải cao và chặn tín hiệu.

### 5. Hàm poll()
- **poll()**: Kiểm tra nhiều file descriptor mà không bị giới hạn như `select()`.

## Chương 6: UDP Sockets

### 1. Giới thiệu về Giao thức UDP
- **UDP**: Giao thức không kết nối, không đáng tin cậy, dùng datagram.
  
### 2. Các Hàm recvfrom và sendto
- **recvfrom() và sendto()**: Nhận và gửi dữ liệu qua UDP.

### 3. Xác minh Phản hồi Nhận được với memcmp()
- **memcmp()**: So sánh dữ liệu trong bộ đệm để xác minh tính hợp lệ của người gửi.

### 4. Sử dụng connect() với UDP
- **connect()**: Dùng để đơn giản hóa gửi và nhận, xử lý lỗi, tăng hiệu suất.

---


