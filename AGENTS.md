# kamae

Windows 向けアクションゲームエンジン。まだ初期で、実行ファイルは `editor` ひとつ。ゲームループ、エンティティ、物理の層はない。このファイルに無い構造を、先回りして作らない。

## 先に読む

- `docs/adr/0001-dx11-imgui-editor-slice.md` — 今の描画とエディタの範囲
- `docs/adr/README.md` — ADR の書き方
- `README.md` — Windows でのビルド

判断の本文は ADR に書く。ここには手順と禁止だけを書く。

## 今の範囲

- C++20、CMake 3.24 以上、実行ファイル名は `editor`
- Win32、DirectX 11、Dear ImGui（docking）、HLSL は `D3DCompile`
- 描くものはライティングした立方体ひとつ。透視カメラ。ビューのドラッグで周回
- ImGui で変えるのはクリア色、立方体の位置と回転、カメラ距離

次を入れるときは、先に ADR を `accepted` にする。それまではコードに入れない。

- DirectX 12、描画バックエンドの抽象化
- ECS、シーン階層、コンポーネント
- 物理、アニメーション、戦闘、音声
- アセットパイプライン、独自 GUI

歩くキャラと追従カメラは、最初の画面が Windows で起動してから。別の変更に混ぜない。

## 変更の手順

1. 機能や設計の変更は GitHub issue から始める。issue に書いていないものを足さない。
2. 描画 API、依存ライブラリ、ディレクトリ構成、ゲームの仕組みを変えるときは `docs/adr/` に ADR を追加する。ファイル名は `NNNN-kebab-title.md`。状態は `proposed` / `accepted` / `superseded`。本文は日本語。
3. 実装は、その issue と ADR の範囲だけ。
4. PR の説明は日本語。何を変えたか、Windows でどうビルドするか、この環境で実行していないならその旨。

## ビルド

Visual Studio 2022 と Windows 10 SDK がある Windows だけでビルドする。手順は README に従う。

Linux やクラウド VM には Windows SDK がない。そこでは構成の確認までに留め、ビルドや起動に成功したと書かない。

## コード

- `src/` — ウィンドウ、ImGui、レンダラ
- `shaders/` — HLSL
- 新しいトップレベルディレクトリは、ADR か issue が場所を決めてから作る
- 見た目とファイルの分け方は、隣のソースに合わせる。このファイルでコーディング規約を新設しない

## 言葉

ADR、issue、PR、README は日本語。識別子と CMake のターゲット名は英語のまま。
