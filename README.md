## Chương 1: Ôn tập Mạng máy tính
### <a name="_egx7yufmih29"></a>**1. Khái niệm Cơ bản về mạng máy tính**
- **Định nghĩa**: Là tập hợp các máy tính (máy trạm, máy chủ, bộ định tuyến) kết nối với nhau để chia sẻ tài nguyên và dữ liệu.
- **Hình dạng Mạng**: Các bố cục phổ biến bao gồm trục (Bus), vòng (Ring), và sao (Star).
- **Giao thức**: Các quy tắc tiêu chuẩn để thiết lập giao tiếp giữa các thiết bị.
### <a name="_zet3lv1sjdr5"></a>**2. Các mô hình OSI và TCP/IP**
- **Kiến trúc phân tầng**: Phân chia các chức năng mạng để tối ưu hóa (như dễ bảo trì và nâng cấp).
- **Mô hình OSI**: Gồm 7 tầng – Vật lý, Liên kết Dữ liệu, Mạng, Giao vận, Phiên, Trình diễn, và Ứng dụng.
- **Mô hình TCP/IP**: Gồm 4 tầng – Ứng dụng, Giao vận (TCP/UDP), Internet (IP), và Giao diện Mạng.
### <a name="_3jdr4yjprt2u"></a>**3. Giao thức IP và định địa chỉ**
- **Chức năng của IP**: Chọn đường đi (Routing) và Chuyển tiếp (Forwarding) dữ liệu.
- **Địa chỉ IPv4**: Địa chỉ 32-bit duy nhất, chia thành ID Mạng và ID Máy trạm.
- **Các Lớp Địa chỉ**: Lớp A, B, C, D, và E, kèm theo CIDR (Định tuyến Không phân lớp) để linh hoạt trong việc phân chia mạng con.
- **Mặt Nạ Mạng**: Tách phần mạng và phần máy trạm trong địa chỉ IP bằng một mặt nạ nhị phân.
### <a name="_vnwbc3bsbz8d"></a>**4. Tầng Giao vận**
- **Vai trò**: Hỗ trợ việc giao tiếp giữa các ứng dụng trên các máy khác nhau.
- **UDP**: Không cần liên kết, nhanh chóng, đơn giản, phù hợp cho các ứng dụng yêu cầu thời gian thực.
- **TCP**: Tin cậy, hướng liên kết, có kiểm soát luồng, kiểm soát tắc nghẽn và kiểm tra lỗi.
- **Bắt tay ba bước**: Thiết lập kết nối TCP bằng các thông điệp SYN, SYN-ACK và ACK.
### <a name="_q7zu60d8x0dl"></a>**5. Tên miền (DNS)**
- **Tên miền**: Địa chỉ dễ nhớ cho con người (như[ ](http://www.example.com)[www.example.com](http://www.example.com)) được ánh xạ với địa chỉ IP.
- **Phân giải Tên**: Chuyển đổi tên miền sang địa chỉ IP thông qua các máy chủ DNS (máy chủ cục bộ và máy chủ gốc).

### <a name="_r40q71rylrg"></a>**6. Tầng ứng dụng**
- **Mô hình Khách-Chủ**: Máy khách gửi yêu cầu tới máy chủ để nhận dịch vụ (ví dụ, Web, Mail).
- **Mô hình Điểm-Điểm (P2P)**: Phân cấp, giao tiếp trực tiếp giữa các nút (ví dụ, Gnutella).
- **Mô hình Lai**: Kết hợp P2P và Khách-Chủ (ví dụ, Skype sử dụng máy chủ trung tâm cho đăng nhập, sau đó chuyển sang P2P).
### <a name="_f1ve8ft3gudj"></a>**7. Giao tiếp giữa các tiến trình và Sockets**
- **Sockets**: Đầu cuối để gửi/nhận dữ liệu, xác định bởi địa chỉ IP và số cổng.
- **Giao tiếp**: Liên quan đến việc trao đổi thông điệp giữa các tiến trình máy khách và máy chủ, tuân theo các giao thức (ví dụ, HTTP, FTP).
- **Giao thức Tầng Ứng dụng**: Xác định các loại thông điệp, cú pháp, ngữ nghĩa và quy tắc giao tiếp.

## Chương 3: Socket APIs
### <a name="_6tl1a14t25na"></a>**1. Khái niệm Cơ bản về Socket**
- **Socket Pair:** Một bộ gồm bốn yếu tố xác định điểm kết nối giữa hai đầu:
  - Địa chỉ IP cục bộ và địa chỉ IP từ xa
  - Cổng cục bộ và cổng từ xa

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.001.png)

- **Cổng TCP và Máy chủ đồng thời**: Cổng TCP cho phép nhiều máy chủ cùng hoạt động, mỗi dịch vụ thường gắn với một cổng đặc biệt.

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.002.png)
### <a name="_2ors7awb8b5"></a>**2. Kích thước bộ đệm và iới hạn**
- **MTU (Đơn vị truyền tối đa):** Kích thước tối đa của gói tin IP là 65,538 byte, tuy nhiên gói tin lớn hơn MTU sẽ bị phân mảnh.
- **MSS (Kích thước phân đoạn tối đa):** Được gửi cho TCP để xác định kích thước tối đa của dữ liệu mà mỗi phân đoạn có thể chứa.
### <a name="_eyr4plt73v5e"></a>**3. Sockets API**
- **Cấu trúc Địa chỉ Socket (IPv4):** Sử dụng cấu trúc sockaddr\_in để xác định địa chỉ IP và cổng.

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.003.png)

- **Tham số Giá trị - Kết quả:** Cấu trúc địa chỉ được truyền tham chiếu và chiều dài cấu trúc cũng là một tham số. Có 2 hướng:
  - Chuyển từ quá trình đến nhân hệ điều hành bao gồm các hàm bind, connect, send

    **Hàm bind( ):** Gắn một địa chỉ IP và cổng vào socket.Cú pháp:


|<p>if (bind(server\_fd, (struct sockaddr \*)&server\_addr, sizeof(server\_addr)) < 0) {</p><p>`        `perror("Bind failed");</p><p>`        `exit(EXIT\_FAILURE);</p><p>`    `}</p>|
| :- |

**Hàm connect( ):** Kết nối socket tới một địa chỉ server cụ thể.

|<p>if (connect(sock, (struct sockaddr \*)&server\_addr, sizeof(server\_addr)) < 0) {</p><p>`        `perror("Connection failed");</p><p>`        `close(sock);</p><p>`        `return -1;</p><p>`    `}</p>|
| :- |

**Hàm send( ):** Gửi dữ liệu qua socket kết nối.

|<p>send(sock, message, strlen(message), 0);</p><p>` `printf("Message sent to server: %s\n", message);</p>|
| :- |



- Chuyển từ nhân đến quá trình bao gồm các hàm  accept, recvfrom, getsockname, getpeername
  ### <a name="_vku6fvw47d8k"></a>**Hàm accept ():** Chấp nhận một kết nối đến từ client.

|<p>if ((new\_socket = accept(server\_fd, (struct sockaddr \*)&client\_addr, &addr\_len)) < 0) {</p><p>`        `perror("Accept failed");</p><p>`        `exit(EXIT\_FAILURE);</p><p>`    `}</p>|
| - |

**Hàm getpeername( ):** Dùng để lấy thông tin địa chỉ của khách hàng.

|<p>if (getpeername(new\_socket, (struct sockaddr \*)&client\_addr, &addr\_len) == -1) {</p><p>`        `perror("getpeername failed");</p><p>`        `exit(EXIT\_FAILURE);</p><p>`    `}</p>|
| :- |

**Hàm recv( ): Nhận dữ liệu từ socket kết nối.**

|ssize\_t recv(int sockfd, void \*buf, size\_t len, int flags);|
| :- |

### <a name="_rht8ic9jj40p"></a>**4. Chức năng chuyển đổi địa chỉ**
- **Chuyển đổi giữa chuỗi ký tự ASCII và giá trị nhị phân:** 
  - **Hàm inet\_aton( ):** Chuyển đổi một chuỗi địa chỉ IPv4 thành giá trị nhị phân

    |int inet\_aton(const char \*strptr, struct in\_addr \*addrptr); |
    | :- |

- **Hàm inet\_addr( ):** Chuyển đổi chuỗi địa chỉ IPv4 thành địa chỉ nhị phân (32-bit) theo thứ tự byte mạng.

  |in\_addr\_t inet\_addr(const char \*strptr); |
  | :- |

- **Hàm inet\_ntoa( ):**  Chuyển đổi địa chỉ nhị phân IPv4 thành chuỗi ký tự dạng “dotted-decimal”.

  |char \*inet\_ntoa(struct in\_addr inaddr);|
  | :- |

- **Hàm inet\_pton( ):** Chuyển đổi địa chỉ IPv4 hoặc IPv6 từ định dạng chuỗi ký tự sang dạng nhị phân.

  |int inet\_pton(int family, const char \*strptr, void \*addrptr); |
  | :- |

- **Hàm inet\_ntop( ):**  Chuyển đổi địa chỉ IPv4 hoặc IPv6 từ định dạng nhị phân sang dạng chuỗi ký tự.

  |const char \*inet\_ntop(int family, const void \*addrptr, char \*strptr, size\_t len); |
  | :- |

- **Hàm sock\_ntop:** Hỗ trợ chuyển đổi địa chỉ từ dạng nhị phân sang chuỗi ký tự, giúp in địa chỉ IP dễ dàng.

  |char \*sock\_ntop(const struct sockaddr \*sockaddr, socklen\_t addrlen);|
  | :- |

### <a name="_hmtg6lishzkh"></a>**5. Hàm Đọc và Ghi**
- **read()**: Dùng để đọc dữ liệu từ một socket, trả về số byte đã đọc, -1 nếu có lỗi, và 0 nếu kết nối đã đóng.

  |ssize\_t read(int sockfd, void \*buf, size\_t count);|
  | :- |

- **write():** Dùng để gửi dữ liệu đến một socket, trả về số byte đã ghi hoặc -1 nếu có lỗi.

  |ssize\_t write(int sockfd, const void \*buf, size\_t count); |
  | :- |

## Chương 4: TCP sockets

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.004.png)

1. **Hàm socket()**
- **Công dụng:** Tạo một socket để giao tiếp mạng.
- **Kết quả trả về:** File descriptor của socket nếu thành công, -1 nếu lỗi.
- **Cú pháp:**

|<p>#include <sys/socket.h></p><p>int socket(int family, int type, int protocol);</p>|
| :- |

1. **Hàm connect()**
- **Công dụng:** Thiết lập kết nối giữa client và server.
- **Kết quả trả về:** 0 nếu thành công, -1 nếu lỗi.
- **Cú pháp:**

|<p>#include <sys/socket.h></p><p>int connect(int sockfd, const struct sockaddr \*servaddr, socklen\_t addrlen);</p>|
| :- |

1. **Nhắc lại: Thiết lập liên kết TCP Giao thức bắt tay 3 bước**

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.005.png)

- Bước 1: A gửi SYN cho B: chỉ ra giá trị khởi tạo seq # của A và không có dữ liệu
- Bước 2: B nhận SYN, trả lời bằng SYN - ACK: B khởi tạo vùng đệm, chỉ ra giá trị khởi tạo seq. # của B 
- Bước 3: A nhận SYNACK, trả lời ACK, có thể kèm theo dữ liệu 
1. **Hàm bind()**
- **Công dụng:** Liên kết socket với một địa chỉ IP và cổng cụ thể trên máy cục bộ.
- **Kết quả trả về**: 0 nếu thành công, -1 nếu lỗi.
- **Cú pháp:**

|<p>#include <sys/socket.h></p><p>int bind(int sockfd, const struct sockaddr \*myaddr, socklen\_t addrlen);</p>|
| :- |

1. **Hàm listen()**
- **Công dụng:** Chuyển socket sang trạng thái chờ kết nối (dành cho server).
- **Kết quả trả về:** 0 nếu thành công, -1 nếu lỗi.
- **Cú pháp:**

|<p>#include <sys/socket.h></p><p>int listen(int sockfd, int backlog);</p>|
| :- |

- Đối số thứ hai (backlog) cho hàm này chỉ định số lượng kết nối tối đa mà kernel sẽ xếp hàng cho socket này.
1. **Hàm accept()**
- **Công dụng:** Server chấp nhận kết nối từ client.
- **Kết quả trả về:** File descriptor của socket kết nối nếu thành công, -1 nếu lỗi.
- **Cú pháp**

|<p>#include <sys/socket.h></p><p>int accept(int sockfd, struct sockaddr \*cliaddr, socklen\_t \*addrlen);</p>|
| :- |

- Các đối số *cliaddr* và *addrlen* được sử dụng để trả về địa chỉ giao thức của tiến trình ngang hàng được kết nối (máy khách).
- *addrlen* là một đối số giá trị - kết quả
1. **Hàm fork()**
- **Công dụng:** Tạo một tiến trình con để xử lý client (đối với server đa tiến trình).
- **Kết quả trả về:** 0 trong tiến trình con, ID của tiến trình con trong tiến trình cha, -1 nếu lỗi.
- **Cú pháp**

|<p>#include <unistd.h></p><p>` `pid\_t fork(void);</p>|
| :- |

- Được trả về một lần trong tiến trình gọi (được gọi là parent) với giá trị trả về là ID tiến trình của tiến trình mới được tạo (child) và cũng trả về một lần trong child, với giá trị trả về là 0.
- Lý do fork trả về 0 trong tiến trình con, thay vì ID tiến trình của tiến trình cha, là vì tiến trình con chỉ có một tiến trình cha và nó luôn có thể lấy ID tiến trình của tiến trình cha bằng cách gọi *getppid*
1. **Hàm close()**
- **Công dụng**: Đóng một socket và kết thúc kết nối TCP.
- **Kết quả trả về**: 0 nếu thành công, -1 nếu lỗi.
- **Cú pháp**

|<p>#include <unistd.h></p><p>` `int close(int sockfd);</p>|
| :- |

- **Tham số sockfd:** File descriptor của socket cần đóng.
- Nếu cha mẹ không đóng socket, khi con đóng socket được kết nối, số tham chiếu của nó sẽ giảm từ 2 xuống 1 và sẽ giữ nguyên ở mức 1 vì cha mẹ không bao giờ đóng socket được kết nối. Điều này sẽ ngăn chặn chuỗi kết thúc kết nối của TCP xảy ra và kết nối sẽ vẫn mở.
1. **Hàm getsockname() và getpeername()**
- **Công dụng:** Lấy địa chỉ socket cục bộ (getsockname) hoặc địa chỉ của đối tác kết nối (getpeername).
- **Kết quả trả về:** 0 nếu thành công, -1 nếu lỗi.
- **Cú pháp**

  |<p>#include <sys/socket.h></p><p>int getsockname(int sockfd, struct sockaddr \*localaddr, socklen\_t \*addrlen);</p><p>int getpeername(int sockfd, struct sockaddr \*peeraddr, socklen\_t \*addrlen);</p>|
  | :- |

- **Tham số**:
  - sockfd: File descriptor của socket.
  - localaddr/ peeraddr: Con trỏ tới cấu trúc địa chỉ để lưu kết quả.
  - addrlen: Con trỏ tới kích thước của cấu trúc địa chỉ.
1. **Máy chủ đồng thời**
- Kết nối máy khách - máy chủ TCP
- Máy chủ là máy chủ đa quy trình đồng thời
- Máy chủ sử dụng hàm fork() để xử lý từng máy khách trong một quy trình con riêng biệt, cho phép quy trình cha tiếp tục chấp nhận các kết nối máy khách mới.
- Trạng thái của máy khách/máy chủ trước khi gọi đến chấp nhận trả lại.

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.006.png)

- Trạng thái của máy khách/máy chủ sau khi trả về từ chấp nhận.

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.007.png)

- Trạng thái của máy khách / máy chủ sau khi fork trả về.

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.008.png)

- Trạng thái của client/server sau khi tiến trình cha và tiến trình con đóng các socket phù hợp.

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.009.png)


## Chương 5: I/O Multiplexing
### <a name="_aby5yms8ib0k"></a>**1. Giới thiệu về I/O Multiplexing**
- **Vấn đề:** Khi một client phải xử lý đồng thời nhiều nguồn dữ liệu (như đầu vào chuẩn và socket TCP), có thể xảy ra tình huống bị khóa khi chờ dữ liệu từ một nguồn (ví dụ: fgets đọc đầu vào chuẩn), trong khi sự kiện từ nguồn khác (như đóng kết nối từ server) không được xử lý kịp thời.
- **Giải pháp:** Sử dụng I/O Multiplexing (đa luồng đầu vào/đầu ra) giúp nhận thông báo từ hệ điều hành khi có một hoặc nhiều điều kiện I/O sẵn sàng, giúp client xử lý các sự kiện khác nhau một cách hiệu quả.
### <a name="_na4xvpwpzvek"></a>**2. Các Mô hình I/O**
- **Blocking I/O:** Chờ đến khi một hoạt động I/O hoàn thành trước khi tiến hành các bước khác.

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.010.png)

- **Non-blocking I/O:** Không chờ hoàn thành, chuyển sang xử lý khác trong khi chờ hoạt động I/O hoàn tất.

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.011.png)

- **I/O Multiplexing:** Dùng các hàm select() và poll() để chờ sự kiện từ nhiều file descriptors.

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.012.png)

- **Signal-driven I/O:** Sử dụng tín hiệu (SIGIO) để thông báo khi một sự kiện I/O sẵn sàng.

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.013.png)

- **Asynchronous I/O:** Dùng hàm aio\_\* theo chuẩn POSIX, hệ điều hành sẽ thông báo khi hoạt động I/O hoàn tất.

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.014.png)

- So sánh các Model

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.015.png)
### <a name="_yayk065jc3hw"></a>**3. Hàm select()**
- **Công dụng:** Cho phép quy trình chờ nhiều sự kiện (như đọc, ghi, hoặc ngoại lệ) từ nhiều file descriptors. Quy trình chỉ tiếp tục khi có ít nhất một sự kiện xảy ra hoặc hết thời gian chờ.
- **Cú pháp:**

|<p>#include <sys/select.h></p><p>int select(int maxfdp1, fd\_set \*readset, fd\_set \*writeset, fd\_set \*exceptset, const struct timeval \*timeout);</p>|
| :- |

- **Tham số:**
  - **readset:** Kiểm tra các file descriptors sẵn sàng để đọc.
  - **writeset:** Kiểm tra các file descriptors sẵn sàng để ghi.
  - **exceptset:** Kiểm tra các file descriptors có điều kiện ngoại lệ.
  - **timeout:** Thời gian chờ tối đa.
- **Kết quả trả về:** Số lượng file descriptors sẵn sàng, 0 nếu hết thời gian chờ, -1 nếu lỗi.
- **Ví dụ minh họa:** Máy chủ một tiến trình sử dụng select()

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.016.png)
### <a name="_udfrj3m44vkg"></a>**4. Hàm pselect()**
- **Cú pháp**

|<p>#include <sys/select.h></p><p>#include <sys/time.h></p><p>#include <signal.h></p><p></p><p>int pselect(int nfds, fd\_set \*readfds, fd\_set \*writefds, fd\_set \*exceptfds,</p><p>`            `const struct timespec \*timeout, const sigset\_t \*sigmask);</p>|
| :- |

- **Khác biệt so với select():**
  - **Signal Masking:** pselect() cho phép thiết lập mặt nạ tín hiệu trước khi chờ và khôi phục sau khi chờ, giúp ngăn chặn điều kiện đua.
  - **Time Resolution:** pselect() sử dụng struct timespec để thiết lập thời gian chờ với độ phân giải cao hơn (tính bằng nanosecond) so với struct timeval của select().
### <a name="_3y4qwjszlxq2"></a>**5. Hàm poll()**
- **Cú pháp**

  |<p>#include <poll.h></p><p></p><p>int poll(struct pollfd \*fds, nfds\_t nfds, int timeout);</p>|
  | :- |

- **Tham số:**
  - **fds:** Mảng các cấu trúc pollfd mô tả file descriptors và các sự kiện mong muốn.
  - **Cấu trúc pollfd:**


|<p>struct pollfd {</p><p>`    `int fd;         // File descriptor cần theo dõi</p><p>`    `short events;   // Các sự kiện cần kiểm tra</p><p>`    `short revents;  // Các sự kiện thực sự xảy ra</p><p>};</p>|
| :- |

- **nfds:** Số phần tử trong mảng fds.
- **timeout:** Thời gian chờ tối đa tính bằng millisecond. Đặt -1 để chờ vô hạn, 0 để kiểm tra ngay lập tức.
- **So sánh poll() và select():**
  - **Giới hạn File Descriptor:** select() bị giới hạn số lượng file descriptors bởi FD\_SETSIZE (thường là 1024), trong khi poll() không có giới hạn này.
  - **Cấu trúc dữ liệu:** poll() sử dụng một mảng có kích thước thay đổi (struct pollfd) để mô tả file descriptor và sự kiện, trong khi select() dùng bitmap cố định.
  - **Hiệu suất:** poll() tối ưu hơn khi có nhiều file descriptors.
  - **Độ chính xác thời gian chờ:** poll() thiết lập thời gian chờ tính bằng millisecond, trong khi select() dùng giây và microsecond.
**


## Chương 6: UDP Sockets
### <a name="_81886d4gchh"></a>**1. Giới thiệu về Giao thức UDP**
- UDP (User Datagram Protocol) là giao thức không kết nối, không đáng tin cậy, sử dụng các gói dữ liệu (datagram) để truyền thông tin.
- Một số ứng dụng phổ biến sử dụng UDP: DNS (Domain Name System), NFS (Network File System), và SNMP (Simple Network Management Protocol).

![](Aspose.Words.944413dd-74b5-4e4a-8545-bf263d9fe9ba.017.png)
### <a name="_pus496vfk739"></a>**2. Các Hàm recvfrom và sendto**
- Được sử dụng để nhận và gửi dữ liệu qua UDP.
- Cú pháp:**


  |<p>#include <sys/socket.h></p><p>ssize\_t recvfrom(int sockfd, void \*buff, size\_t nbytes, int flags, struct sockaddr \*from, socklen\_t \*addrlen);</p><p></p><p>ssize\_t sendto(int sockfd, const void \*buff, size\_t nbytes, int flags, const struct sockaddr \*to, socklen\_t addrlen);</p>|
  | :- |

- Tham số:

  - sockfd: File descriptor của socket.
  - buff: Bộ đệm dữ liệu.
  - nbytes: Kích thước của dữ liệu.
  - from/to: Địa chỉ của sender hoặc receiver.
  - addrlen: Kích thước của địa chỉ.
- Kết quả trả về: Số byte đã đọc hoặc ghi, -1 nếu có lỗi.
- Lưu ý: Có thể gửi gói dữ liệu với độ dài 0.
### <a name="_nwnzkhsxma3k"></a>**3. Xác minh Phản hồi Nhận được với memcmp()**
- Do UDP không kết nối nên cần cơ chế xác minh tính hợp lệ của người gửi.
- Hàm memcmp() giúp so sánh các dữ liệu trong bộ đệm để kiểm tra tính nhất quán của gói tin nhận được.
### <a name="_48k8y4f1tmey"></a>**4. Sử dụng connect() với UDP**
- Mặc dù UDP không kết nối, connect() có thể được sử dụng cho một số mục đích:
  - Đơn giản hóa Gửi và Nhận: Sau khi gọi connect() trên một socket UDP, có thể dùng send() và recv() thay vì sendto() và recvfrom().
  - Khóa Địa chỉ Peer: Xác định duy nhất địa chỉ IP và cổng từ xa, tạo tính nhất quán trong giao tiếp.
  - Xử lý Lỗi: Nhận thông tin phản hồi ICMP khi không thể truyền tải đến peer.
  - Tăng Hiệu Suất: Trong một số trường hợp, connect() có thể giúp tối ưu hóa tốc độ giao tiếp.







