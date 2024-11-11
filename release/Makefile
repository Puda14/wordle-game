# Định nghĩa các đường dẫn đến thư mục nguồn
APP_DIR = ../app
MODEL_DIR = ../model
GUI_DIR = ../gui

# Compiler và flags
CC = gcc
CFLAGS = -I$(MODEL_DIR) -I$(GUI_DIR)
GTK_FLAGS = `pkg-config --cflags --libs gtk4`

# Tên của các tệp thực thi
TARGETS = server client

# Các tệp đối tượng cho server và client
SERVER_OBJS = server.o message.o user.o
CLIENT_OBJS = client.o message.o user.o client_gui_helpers.o

# Quy tắc mặc định: tạo cả server và client
all: $(TARGETS)

# Quy tắc để tạo server
server: $(SERVER_OBJS)
	$(CC) -o $@ $(SERVER_OBJS)

# Quy tắc để tạo client (thêm GTK_FLAGS)
client: $(CLIENT_OBJS)
	$(CC) -o $@ $(CLIENT_OBJS) $(GTK_FLAGS)

# Quy tắc để biên dịch server.o từ server.c
server.o: $(APP_DIR)/server.c $(MODEL_DIR)/message.h $(MODEL_DIR)/user.h
	$(CC) $(CFLAGS) -c $(APP_DIR)/server.c -o $@

# Quy tắc để biên dịch client.o từ client.c (thêm GTK_FLAGS)
client.o: $(APP_DIR)/client.c $(MODEL_DIR)/message.h $(MODEL_DIR)/user.h $(GUI_DIR)/client_gui_helpers.h
	$(CC) $(CFLAGS) $(GTK_FLAGS) -c $(APP_DIR)/client.c -o $@

# Quy tắc để biên dịch message.o từ message.c
message.o: $(MODEL_DIR)/message.c $(MODEL_DIR)/message.h
	$(CC) $(CFLAGS) -c $(MODEL_DIR)/message.c -o $@

# Quy tắc để biên dịch user.o từ user.c
user.o: $(MODEL_DIR)/user.c $(MODEL_DIR)/user.h
	$(CC) $(CFLAGS) -c $(MODEL_DIR)/user.c -o $@

# Quy tắc để biên dịch client_gui_helpers.o từ client_gui_helpers.c (thêm message.h)
client_gui_helpers.o: $(GUI_DIR)/client_gui_helpers.c $(GUI_DIR)/client_gui_helpers.h $(MODEL_DIR)/message.h
	$(CC) $(CFLAGS) $(GTK_FLAGS) -c $(GUI_DIR)/client_gui_helpers.c -o $@

# Quy tắc dọn dẹp
clean:
	rm -f *.o $(TARGETS)
