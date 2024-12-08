#include <gtk/gtk.h>
#include <string.h>

#define WORD_LENGTH 5
#define MAX_ATTEMPTS 12

typedef struct {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *entry;
    GtkWidget *send_button;
    GtkWidget *status_label;
    GtkWidget *cells[MAX_ATTEMPTS][WORD_LENGTH];
    int current_attempt;
} AppWidgets;

// Hàm hiển thị đoán và phản hồi lên bảng
void display_feedback(AppWidgets *app, const char *guess, const char *feedback) {
    for (int i = 0; i < WORD_LENGTH; i++) {
        gtk_label_set_text(GTK_LABEL(app->cells[app->current_attempt][i]), g_strndup(&guess[i], 1));
        // Gán lớp CSS cho từng ô dựa trên phản hồi
        GtkStyleContext *context = gtk_widget_get_style_context(app->cells[app->current_attempt][i]);
        if (feedback[i] == 'G') {
            gtk_style_context_add_class(context, "correct");
        } else if (feedback[i] == 'Y') {
            gtk_style_context_add_class(context, "misplaced");
        } else {
            gtk_style_context_add_class(context, "incorrect");
        }
    }
    app->current_attempt++;
}

// Hàm xử lý khi nhấn nút gửi đoán
void send_guess(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;
    const char *guess = gtk_entry_get_text(GTK_ENTRY(app->entry));

    if (strlen(guess) != WORD_LENGTH) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Please enter a 5-letter word.");
        return;
    }

    // Giả lập phản hồi từ server (feedback)
    const char *target_word = "apple"; // Từ mục tiêu tạm thời
    char feedback[WORD_LENGTH + 1] = {0};

    for (int i = 0; i < WORD_LENGTH; i++) {
        if (guess[i] == target_word[i]) {
            feedback[i] = 'G';
        } else if (strchr(target_word, guess[i]) != NULL) {
            feedback[i] = 'Y';
        } else {
            feedback[i] = 'B';
        }
    }

    // Hiển thị phản hồi lên bảng
    display_feedback(app, guess, feedback);

    // Kiểm tra thắng/thua
    if (strcmp(guess, target_word) == 0) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Congratulations! You guessed the word!");
        gtk_widget_set_sensitive(app->send_button, FALSE);
    } else if (app->current_attempt >= MAX_ATTEMPTS) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Game over! You've used all attempts.");
        gtk_widget_set_sensitive(app->send_button, FALSE);
    } else {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Keep guessing!");
    }

    gtk_entry_set_text(GTK_ENTRY(app->entry), ""); // Xóa ô nhập sau khi gửi
}

// Hàm áp dụng CSS
void apply_css() {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "label {"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  border: 2px solid black;"
        "  padding: 10px;"
        "  border-radius: 5px;"
        "  text-align: center;"
        "}"
        ".correct {"
        "  background-color: #00FF00;" // Xanh lá - đúng vị trí
        "  color: white;"
        "}"
        ".misplaced {"
        "  background-color: #FFFF00;" // Vàng - sai vị trí
        "  color: black;"
        "}"
        ".incorrect {"
        "  background-color: #808080;" // Xám - sai hoàn toàn
        "  color: white;"
        "}", -1, NULL);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
}

// Tạo giao diện GUI
void create_gui(AppWidgets *app) {
    // Tạo cửa sổ chính
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "Wordle Game");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 400, 600);

    // Tạo lưới chính
    app->grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(app->window), app->grid);

    // Tạo bảng 12x5 cho từ đoán
    for (int i = 0; i < MAX_ATTEMPTS; i++) {
        for (int j = 0; j < WORD_LENGTH; j++) {
            app->cells[i][j] = gtk_label_new("");
            gtk_widget_set_size_request(app->cells[i][j], 50, 50);
            gtk_grid_attach(GTK_GRID(app->grid), app->cells[i][j], j, i, 1, 1);

            // Gán tên lớp CSS mặc định cho từng ô
            GtkStyleContext *context = gtk_widget_get_style_context(app->cells[i][j]);
            gtk_style_context_add_class(context, "cell");
        }
    }

    // Tạo ô nhập từ đoán
    app->entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(app->entry), WORD_LENGTH);
    gtk_grid_attach(GTK_GRID(app->grid), app->entry, 0, MAX_ATTEMPTS, WORD_LENGTH - 1, 1);

    // Tạo nút gửi đoán
    app->send_button = gtk_button_new_with_label("Send Guess");
    gtk_grid_attach(GTK_GRID(app->grid), app->send_button, WORD_LENGTH - 1, MAX_ATTEMPTS, 1, 1);

    // Tạo nhãn trạng thái
    app->status_label = gtk_label_new("Welcome to Wordle!");
    gtk_grid_attach(GTK_GRID(app->grid), app->status_label, 0, MAX_ATTEMPTS + 1, WORD_LENGTH, 1);

    // Kết nối sự kiện nút gửi đoán
    g_signal_connect(app->send_button, "clicked", G_CALLBACK(send_guess), app);

    // Hiển thị tất cả các widget
    gtk_widget_show_all(app->window);

    // Đóng cửa sổ khi thoát
    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    AppWidgets app = {0};
    app.current_attempt = 0;

    apply_css(); // Áp dụng CSS trước
    create_gui(&app);

    gtk_main();
    return 0;
}
