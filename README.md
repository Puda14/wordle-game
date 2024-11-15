# wordle-game

This is a project for the network programming practice course.

## Project structure

```sh
.
├── app
│   ├── client.c
│   ├── server.c
│   └── users.txt
├── GUI
│   ├── gui
│   └── gui.c
├── model
│   ├── message.c
│   ├── message.h
│   ├── user.c
│   └── user.h
├── READM
```

**To run**
```sh
cd release
make
```

Compile GUI:
```c
gcc gui.c -o gui `pkg-config --cflags --libs gtk4`
```

`pkg-config --cflags --libs gtk4`:
- `pkg-config`: Dùng để tìm kiếm và lấy các thông tin cần thiết về thư viện GTK (các thông tin cờ biên dịch và đường dẫn thư viện).
- `--cflags`: Lấy các cờ biên dịch cần thiết.
- `--libs`: Lấy các đường dẫn liên kết tới các thư viện cần thiết cho GTK.
gtk4: Phiên bản của GTK mà bạn đang sử dụng.

```sh
+----------------------- Window (vbox) ----------------------+
|                     Wordle Game GUI                        |
|                                                            |
| +-------------- hbox_main (Horizontal Box) ---------------+|
| |                                                         ||
| | +--- VBox (You) ---+   +--- VBox (Opponent) ---+        ||
| | |                  |   |                       |        ||
| | |  Label: "YOU"    |   |   Label: "OPPONENT"   |        ||
| | | +--------------+ |   |   +---------------+   |        ||
| | | | 6x5 Grid     | |   |   | 6x5 Grid      |   |        ||
| | | +--------------+ |   |   +---------------+   |        ||
| | +------------------+   +-----------------------+        ||
| +---------------------------------------------------------+|
|                                                            |
| +------------ Submit Box (Horizontal Box) --------------+  |
| | Entry Field                 [Submit Button]           |  |
| +-------------------------------------------------------+  |
+------------------------------------------------------------+
```
