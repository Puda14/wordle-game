#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define WORD_LENGTH 5
#define MAX_ATTEMPTS 12
#define MAX_USERS 100

typedef struct {
    GtkWidget *main_window;
    GtkWidget *menu_window;
    GtkWidget *game_window;
    GtkWidget *grid;
    GtkWidget *entry;
    GtkWidget *send_button;
    GtkWidget *status_label;
    GtkWidget *list_box;
    GtkWidget *back_button;
    GtkWidget *challenge_label;
    GtkWidget *challenge_button;
    GtkWidget *back_to_menu_button;

    GtkWidget *cells[MAX_ATTEMPTS][WORD_LENGTH];
    int current_attempt;

    char target_word[WORD_LENGTH + 1];
    char users[MAX_USERS][50];
    int user_count;
} AppWidgets;

// Giả lập danh sách người dùng
void load_users(AppWidgets *app) {
    FILE *file = fopen("users.txt", "r");
    if (!file) {
        perror("Error opening users.txt");
        exit(1);
    }
    app->user_count = 0;
    while (fscanf(file, "%49s", app->users[app->user_count]) != EOF && app->user_count < MAX_USERS) {
        app->user_count++;
    }
    fclose(file);
}

// Hàm hiển thị đoán và phản hồi lên bảng
void display_feedback(AppWidgets *app, const char *guess, const char *feedback) {
    for (int i = 0; i < WORD_LENGTH; i++) {
        gtk_label_set_text(GTK_LABEL(app->cells[app->current_attempt][i]), g_strndup(&guess[i], 1));
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

    char feedback[WORD_LENGTH + 1] = {0};
    for (int i = 0; i < WORD_LENGTH; i++) {
        if (guess[i] == app->target_word[i]) {
            feedback[i] = 'G';
        } else if (strchr(app->target_word, guess[i]) != NULL) {
            feedback[i] = 'Y';
        } else {
            feedback[i] = 'B';
        }
    }

    display_feedback(app, guess, feedback);

    if (strcmp(guess, app->target_word) == 0) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Congratulations! You guessed the word!");
        gtk_widget_set_sensitive(app->send_button, FALSE);
    } else if (app->current_attempt >= MAX_ATTEMPTS) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Game over! You've used all attempts.");
        gtk_widget_set_sensitive(app->send_button, FALSE);
    } else {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Keep guessing!");
    }

    gtk_entry_set_text(GTK_ENTRY(app->entry), "");
}

// Hàm gửi thách đấu
void send_challenge(GtkWidget *widget, gpointer data) {
    AppWidgets *app = (AppWidgets *)data;

    // Giả lập chọn ngẫu nhiên một người dùng
    int random_user_index = rand() % app->user_count;
    const char *opponent = app->users[random_user_index];

    // Hiển thị thông báo gửi thách đấu
    char message[100];
    snprintf(message, sizeof(message), "You challenged %s!", opponent);
    gtk_label_set_text(GTK_LABEL(app->challenge_label), message);

    // Mở cửa sổ game sau 2 giây (giả lập đối thủ đồng ý)
    gtk_widget_hide(app->menu_window);
    gtk_widget_show_all(app->game_window);

    // Tạo từ mục tiêu ngẫu nhiên
    strncpy(app->target_word, "apple", WORD_LENGTH); // Thay bằng từ ngẫu nhiên nếu cần
    app->target_word[WORD_LENGTH] = '\0';
}

// Tạo giao diện màn hình chơi game
void create_game_gui(AppWidgets *app) {
    app->game_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->game_window), "Wordle Game");
    gtk_window_set_default_size(GTK_WINDOW(app->game_window), 400, 600);

    app->grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(app->game_window), app->grid);

    for (int i = 0; i < MAX_ATTEMPTS; i++) {
        for (int j = 0; j < WORD_LENGTH; j++) {
            app->cells[i][j] = gtk_label_new("");
            gtk_widget_set_size_request(app->cells[i][j], 50, 50);
            gtk_grid_attach(GTK_GRID(app->grid), app->cells[i][j], j, i, 1, 1);
        }
    }

    app->entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(app->entry), WORD_LENGTH);
    gtk_grid_attach(GTK_GRID(app->grid), app->entry, 0, MAX_ATTEMPTS, WORD_LENGTH - 1, 1);

    app->send_button = gtk_button_new_with_label("Send Guess");
    gtk_grid_attach(GTK_GRID(app->grid), app->send_button, WORD_LENGTH - 1, MAX_ATTEMPTS, 1, 1);
    g_signal_connect(app->send_button, "clicked", G_CALLBACK(send_guess), app);

    app->status_label = gtk_label_new("Welcome to Wordle!");
    gtk_grid_attach(GTK_GRID(app->grid), app->status_label, 0, MAX_ATTEMPTS + 1, WORD_LENGTH, 1);

    gtk_widget_hide(app->game_window);
}

// Tạo giao diện menu chính
void create_menu_gui(AppWidgets *app) {
    app->menu_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->menu_window), "Wordle Menu");
    gtk_window_set_default_size(GTK_WINDOW(app->menu_window), 400, 300);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(app->menu_window), vbox);

    app->challenge_button = gtk_button_new_with_label("Send Challenge");
    gtk_box_pack_start(GTK_BOX(vbox), app->challenge_button, FALSE, FALSE, 0);
    g_signal_connect(app->challenge_button, "clicked", G_CALLBACK(send_challenge), app);

    app->challenge_label = gtk_label_new("Ready to challenge!");
    gtk_box_pack_start(GTK_BOX(vbox), app->challenge_label, FALSE, FALSE, 0);

    gtk_widget_show_all(app->menu_window);
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
        "  background-color: #00FF00;"
        "  color: white;"
        "}"
        ".misplaced {"
        "  background-color: #FFFF00;"
        "  color: black;"
        "}"
        ".incorrect {"
        "  background-color: #808080;"
        "  color: white;"
        "}", -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    srand(time(NULL)); // Tạo seed ngẫu nhiên cho thách đấu

    AppWidgets app = {0};
    app.current_attempt = 0;

    load_users(&app);
    apply_css();
    create_menu_gui(&app);
    create_game_gui(&app);

    gtk_main();
    return 0;
}
