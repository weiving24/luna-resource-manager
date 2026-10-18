[English](README.md) | [简体中文](README.zh-CN.md) | [繁體中文](README.zh-TW.md) | [日本語](README.ja.md)

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
![C99](https://img.shields.io/badge/C-C99-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS-lightgrey.svg)
![UI](https://img.shields.io/badge/UI-curses-green.svg)
![Archive](https://img.shields.io/badge/Archive-libarchive-orange.svg)

# Luna Resource Manager

Luna Resource Manager は、C 言語で書かれたモジュール式のターミナルファイルマネージャーです。ファイルシステムとプロセス管理のコードは POSIX API を使用し、インタラクティブなターミナルインターフェースは curses ライブラリ（ncurses、ncursesw、または互換実装）を使用しています。

## ビルド要件

- C99 をサポートする C コンパイラ
- POSIX.1-2008 互換のシステムインターフェース
- `make`
- curses 開発ライブラリとヘッダーファイル
- libarchive 開発ヘッダーとライブラリ（アーカイブプレビュー用）
- `pkg-config` はオプションです。`CURSES_CFLAGS` と `CURSES_LIBS` を手動で指定することもできます。

Debian または Ubuntu では、以下のコマンドでビルド依存関係をインストールします：

```sh
sudo apt install build-essential libncurses-dev libarchive-dev pkg-config
```

ビルドとテスト：

```sh
make
make test
```

実行：

```sh
./bin/luna-resource-manager [directory]
./bin/luna-resource-manager --help
./bin/luna-resource-manager --config /path/to/config.conf /path/to/directory
```

## 設定

設定ファイルは、以下の順序で最初に見つかった場所から読み込まれます：`--config FILE`、`LUNA_RESOURCE_MANAGER_CONFIG`、`$XDG_CONFIG_HOME/luna-resource-manager/config.conf`（または `$HOME/.config/luna-resource-manager/config.conf`）、`/etc/luna-resource-manager.conf`、そしてカレントディレクトリからの `config/theme.conf`。

例：

```ini
theme=default
panel_ratio=55
show_header=1
show_preview=1
show_statusbar=1
```

テーマは `default`、`dark`、`light` から選択できます。ブール値は `0/1`、`yes/no`、`true/false`、または `on/off` を受け付けます。`panel_ratio` は 20 から 80 の間である必要があります。

## キーバインド

- 上 / 下、Page Up / Page Down、Home / End：ナビゲーション
- Enter / 右矢印キー：ディレクトリに入る、またはファイルを開く
- 左矢印キー：親ディレクトリに移動
- F2 または マウスの右クリック：選択項目のアクションメニューを開く
- サポートされているアーカイブを右クリックすると、プレビュー画面が直接開きます。F2 → Open → Preview archive でも可能です。
- アーカイブプレビューは、ZIP、TAR、圧縮 TAR、7z アーカイブ内のエントリを一覧表示します（libarchive を使用）
- `n`：ディレクトリを作成
- `a`：バージョン情報
- `?` / F1：ヘルプ
- `q`：終了

内蔵のテキストビューアは通常のテキストファイルを処理します。実行可能ファイルは、制限されたバッファ内で出力をキャプチャして実行できます。エディタ、ページャー、システムオープナーを起動する際、ファイルパスはシェルを介して渡されません。`xdg-open` や macOS の `open` が利用できないシステムでは、`LUNA_RESOURCE_MANAGER_OPEN` を設定してシステムオープナーを指定できます。

## ソースコード構成

- `app/main.c`：`main()` のみを含み、`app_run()` に委譲します
- `app/app.c`：コマンドライン処理とアプリケーションの起動
- `app/controller.c`：イベントループ、ナビゲーション、カーソルとスクロールの状態
- `app/config.c`：設定の解析とテーマの選択
- `app/dialogs.c`、`app/menu.c`、`app/dir_picker.c`：ターミナルインタラクションコンポーネント
- `app/fileops.c`：ユーザー向けのファイル操作
- `app/open.c`、`app/process.c`、`app/viewer.c`、`app/archive_preview.c`、`app/help.c`：ファイルのオープン、プロセスの起動、表示、アーカイブリスト
- `app/state.c`、`app/callbacks.c`：アプリケーションの状態と UI コールバック
- `core/fs.c`：ディレクトリリスト、パスの結合、プレビューの読み取り
- `core/fs_ops.c`：再帰的なコピー、移動、削除
- `uidl/`：UI レイアウトモデルとテーマ
- `prt/`：curses レンダラー
- `tests/test_fs.c`：ファイルシステムの回帰テスト

`make clean` コマンドは、生成されたバイナリとオブジェクトファイルを削除します。ソースアーカイブにはビルド成果物が意図的に含まれていません。
