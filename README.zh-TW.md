[English](README.md) | [简体中文](README.zh-CN.md) | [繁體中文](README.zh-TW.md) | [日本語](README.ja.md)

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
![C99](https://img.shields.io/badge/C-C99-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS-lightgrey.svg)
![UI](https://img.shields.io/badge/UI-curses-green.svg)
![Archive](https://img.shields.io/badge/Archive-libarchive-orange.svg)

# Luna Resource Manager

Luna Resource Manager 是一個以 C 語言編寫的模組化終端機檔案管理員。其檔案系統與行程管理程式碼使用 POSIX API；互動式終端機介面使用 curses 函式庫（ncurses、ncursesw 或相容實作）。

## 建置需求

- 支援 C99 的 C 編譯器
- 相容於 POSIX.1-2008 的系統介面
- `make`
- curses 開發函式庫與標頭檔
- libarchive 開發標頭檔與函式庫（用於壓縮檔預覽）
- `pkg-config` 為選用；可手動提供 `CURSES_CFLAGS` 與 `CURSES_LIBS`

在 Debian 或 Ubuntu 上，使用以下指令安裝建置相依套件：

```sh
sudo apt install build-essential libncurses-dev libarchive-dev pkg-config
```

建置與測試：

```sh
make
make test
```

執行：

```sh
./bin/luna-resource-manager [directory]
./bin/luna-resource-manager --help
./bin/luna-resource-manager --config /path/to/config.conf /path/to/directory
```

## 設定

設定檔的讀取順序為（優先級由高至低）：`--config FILE`、`LUNA_RESOURCE_MANAGER_CONFIG`、`$XDG_CONFIG_HOME/luna-resource-manager/config.conf`（或 `$HOME/.config/luna-resource-manager/config.conf`）、`/etc/luna-resource-manager.conf`，以及相對於目前目錄的 `config/theme.conf`。

範例：

```ini
theme=default
panel_ratio=55
show_header=1
show_preview=1
show_statusbar=1
```

主題可選 `default`、`dark` 或 `light`。布林值接受 `0/1`、`yes/no`、`true/false` 或 `on/off`。`panel_ratio` 必須介於 20 到 80 之間。

## 快速鍵

- 上 / 下，Page Up / Page Down，Home / End：導覽
- Enter / 右方向鍵：進入目錄或開啟檔案
- 左方向鍵：返回上一層目錄
- F2 或 滑鼠右鍵：開啟選取項目的操作選單
- 右鍵點擊支援的壓縮檔可直接開啟預覽介面；F2 → Open → Preview archive 也可實現
- 壓縮檔預覽支援列出 ZIP、TAR、壓縮 TAR 以及 7z 格式的內容（使用 libarchive）
- `n`：建立目錄
- `a`：關於
- `?` / F1：說明
- `q`：離開

內建的文字檢視器可處理一般文字檔；可執行檔可在有界緩衝區中擷取輸出並執行。啟動編輯器、分頁器與系統開啟程式時，不會透過 shell 傳遞檔案路徑。在不支援 `xdg-open` 或 macOS `open` 的系統上，可設定 `LUNA_RESOURCE_MANAGER_OPEN` 來選擇系統開啟程式。

## 原始碼結構

- `app/main.c`：僅包含 `main()`，並委派給 `app_run()`
- `app/app.c`：命令列處理與應用程式啟動
- `app/controller.c`：事件迴圈、導覽、游標與捲動狀態
- `app/config.c`：設定解析與主題選擇
- `app/dialogs.c`、`app/menu.c`、`app/dir_picker.c`：終端機互動元件
- `app/fileops.c`：面向使用者的檔案操作
- `app/open.c`、`app/process.c`、`app/viewer.c`、`app/archive_preview.c`、`app/help.c`：檔案開啟、行程啟動、檢視與壓縮檔列表展示
- `app/state.c`、`app/callbacks.c`：應用程式狀態與 UI 回呼
- `core/fs.c`：目錄列表、路徑拼接與預覽讀取
- `core/fs_ops.c`：遞迴複製、移動與刪除
- `uidl/`：UI 版面模型與主題
- `prt/`：curses 渲染器
- `tests/test_fs.c`：檔案系統迴歸測試

`make clean` 指令會刪除產生的二進位檔與目的檔。原始碼分发包有意排除了建置產物。
