#include <gtk/gtk.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_USERS 100

typedef struct {
    int message_type;
    char payload[BUFFER_SIZE];
} Message;

enum MessageType {
    SIGN_UP = 0,
    LOGIN = 1,
    LOGOUT = 2,
    LIST_USERS = 3,
    CHALLENGE_REQUEST = 4
};

// Cấu trúc chứa thông tin người dùng
typedef struct {
    char username[50];
    int status; // 0: Offline, 1: Online
} User;

typedef struct {
    GtkWidget *menu_window;
    GtkWidget *list_window;
    GtkWidget *list_box;
    GtkWidget *back_button;
    int sock;
} AppWidgets;

// Gửi thông điệp đến server
void send_message(AppWidgets *app, int message_type, const char *payload) {
    Message message;
    message.message_type = message_type;
    strncpy(message.payload, payload, BUFFER_SIZE - 1);
    send(app->sock, &message, sizeof(message), 0);
}

// Nhận phản hồi từ server
void receive_message(AppWidgets *app, Message *message) {
    recv(app->sock, message, sizeof(*message), 0);
}

// Hàm để gửi thách đấu đến một người dùng
void send_challenge(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    const char *opponent = gtk_button_get_label(GTK_BUTTON(widget));

    // Gửi yêu cầu thách đấu đến server
    send_message(app, CHALLENGE_REQUEST, opponent);

    // Nhận phản hồi từ server
    Message response;
    receive_message(app, &response);

    // Hiển thị thông báo
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->list_window),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "%s", response.payload);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Hàm xử lý khi nhấn nút "Back"
void back_to_menu(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    gtk_widget_hide(app->list_window);
    gtk_widget_show_all(app->menu_window);
}

// Hiển thị danh sách người dùng
void on_list_users(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;

    // Gửi yêu cầu danh sách người dùng đến server
    send_message(app, LIST_USERS, "");

    // Nhận phản hồi từ server
    Message response;
    receive_message(app, &response);

    // Hiển thị danh sách người dùng trong list_box
    gtk_container_foreach(GTK_CONTAINER(app->list_box), (GtkCallback)gtk_widget_destroy, NULL);

    char *line = strtok(response.payload, "\n");
    while (line != NULL) {
        // Tạo nút cho mỗi người dùng
        GtkWidget *user_button = gtk_button_new_with_label(line);
        g_signal_connect(user_button, "clicked", G_CALLBACK(send_challenge), app);
        gtk_box_pack_start(GTK_BOX(app->list_box), user_button, FALSE, FALSE, 0);
        line = strtok(NULL, "\n");
    }

    gtk_widget_show_all(app->list_window);
    gtk_widget_hide(app->menu_window);
}

// Tạo giao diện danh sách người dùng
void create_list_users_gui(AppWidgets *app) {
    app->list_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->list_window), "User List");
    gtk_window_set_default_size(GTK_WINDOW(app->list_window), 400, 300);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(app->list_window), vbox);

    app->list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), app->list_box, TRUE, TRUE, 0);

    app->back_button = gtk_button_new_with_label("Back");
    gtk_box_pack_start(GTK_BOX(vbox), app->back_button, FALSE, FALSE, 0);

    g_signal_connect(app->back_button, "clicked", G_CALLBACK(back_to_menu), app);

    gtk_widget_hide(app->list_window); // Ẩn cửa sổ danh sách ban đầu
}

// Tạo giao diện menu chính
void create_menu_gui(AppWidgets *app) {
    app->menu_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->menu_window), "Wordle Menu");
    gtk_window_set_default_size(GTK_WINDOW(app->menu_window), 400, 300);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(app->menu_window), vbox);

    GtkWidget *list_users_button = gtk_button_new_with_label("List Users");
    gtk_box_pack_start(GTK_BOX(vbox), list_users_button, FALSE, FALSE, 0);

    g_signal_connect(list_users_button, "clicked", G_CALLBACK(on_list_users), app);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    AppWidgets app;
    memset(&app, 0, sizeof(app));

    // Kết nối đến server
    app.sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(app.sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    // Tạo giao diện
    create_menu_gui(&app);
    create_list_users_gui(&app);

    gtk_widget_show_all(app.menu_window);
    gtk_main();

    close(app.sock);
    return 0;
}
