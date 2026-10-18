[English](README.md) | [简体中文](README.zh-CN.md) | [繁體中文](README.zh-TW.md) | [日本語](README.ja.md)

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
![C99](https://img.shields.io/badge/C-C99-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS-lightgrey.svg)
![UI](https://img.shields.io/badge/UI-curses-green.svg)
![Archive](https://img.shields.io/badge/Archive-libarchive-orange.svg)

# Luna Resource Manager

Luna Resource Manager 是一个用 C 语言编写的模块化终端文件管理器。其文件系统和进程管理代码使用 POSIX API；交互式终端界面使用 curses 库（ncurses、ncursesw 或兼容实现）。

## 构建要求

- 支持 C99 的 C 编译器
- 兼容 POSIX.1-2008 的系统接口
- `make`
- curses 开发库和头文件
- libarchive 开发头文件和库（用于压缩包预览）
- `pkg-config` 是可选的；可以手动提供 `CURSES_CFLAGS` 和 `CURSES_LIBS`

在 Debian 或 Ubuntu 上，使用以下命令安装构建依赖：

```sh
sudo apt install build-essential libncurses-dev libarchive-dev pkg-config
```

构建与测试：

```sh
make
make test
```

运行：

```sh
./bin/luna-resource-manager [directory]
./bin/luna-resource-manager --help
./bin/luna-resource-manager --config /path/to/config.conf /path/to/directory
```

## 配置

配置文件的读取顺序为（按优先级从高到低）：`--config FILE`、`LUNA_RESOURCE_MANAGER_CONFIG`、`$XDG_CONFIG_HOME/luna-resource-manager/config.conf`（或 `$HOME/.config/luna-resource-manager/config.conf`）、`/etc/luna-resource-manager.conf`，以及相对于当前目录的 `config/theme.conf`。

示例：

```ini
theme=default
panel_ratio=55
show_header=1
show_preview=1
show_statusbar=1
```

主题可选 `default`、`dark` 或 `light`。布尔值接受 `0/1`、`yes/no`、`true/false` 或 `on/off`。`panel_ratio` 必须在 20 到 80 之间。

## 快捷键

- 上 / 下，Page Up / Page Down，Home / End：导航
- Enter / 右方向键：进入目录或打开文件
- 左方向键：返回上一级目录
- F2 或 鼠标右键：打开选中项的操作菜单
- 右键点击受支持的压缩包可直接打开预览界面；F2 → Open → Preview archive 也可实现
- 压缩包预览支持列出 ZIP、TAR、压缩 TAR 以及 7z 格式的内容（使用 libarchive）
- `n`：创建目录
- `a`：关于
- `?` / F1：帮助
- `q`：退出

内置的文本查看器可以处理普通文本文件；可执行文件可以在有界缓冲区中捕获输出并运行。启动编辑器、分页器和系统打开程序时，不会通过 shell 传递文件路径。在不支持 `xdg-open` 或 macOS `open` 的系统上，可以设置 `LUNA_RESOURCE_MANAGER_OPEN` 来选择系统打开程序。

## 源码结构

- `app/main.c`：仅包含 `main()`，并委托给 `app_run()`
- `app/app.c`：命令行处理和应用程序启动
- `app/controller.c`：事件循环、导航、光标和滚动状态
- `app/config.c`：配置解析和主题选择
- `app/dialogs.c`、`app/menu.c`、`app/dir_picker.c`：终端交互组件
- `app/fileops.c`：面向用户的文件操作
- `app/open.c`、`app/process.c`、`app/viewer.c`、`app/archive_preview.c`、`app/help.c`：文件打开、进程启动、查看和压缩包列表展示
- `app/state.c`、`app/callbacks.c`：应用程序状态和 UI 回调
- `core/fs.c`：目录列表、路径拼接和预览读取
- `core/fs_ops.c`：递归复制、移动和删除
- `uidl/`：UI 布局模型和主题
- `prt/`：curses 渲染器
- `tests/test_fs.c`：文件系统回归测试

`make clean` 命令会删除生成的二进制文件和目标文件。源码分发包有意排除了构建产物。
