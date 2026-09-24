# editor

Windows 向けアクションゲームエンジンの、最初の画面です。実行ファイルはひとつだけで、名前は `editor` です。Win32 ウィンドウに DirectX 11 で、同じ立方体をプレイヤーと相手の二人分、その下の床と一緒に描きます。色は主体ごとに一色で、面ごとの色はありません。Dear ImGui のドッキングでクリア色、プレイヤーの位置と回転、カメラ距離をその場で変えられます。ビューポート上の左ドラッグで、カメラはプレイヤーの周りを回ります。ビューポートにマウスがあるとき、WASD でプレイヤーが床の上を歩きます。相手は初期位置 `(0, 0, 4)` から動きません。既定の前後左右はプレイヤーのヨーで、歩行はヨーを変えません。向きは Player パネルの回転だけです。Camera パネルで切り替えると、前後左右はカメラの水平なヨーになり、移動中だけプレイヤーがその方向を向きます。歩行は、プレイヤーの底が床の板から出るところで止まります。相手にはこの停止を適用しません。重力で落ちることはしません。

物理ライブラリ、アニメーション、戦闘は含めていません。判断の記録は [docs/adr/0001-dx11-imgui-editor-slice.md](docs/adr/0001-dx11-imgui-editor-slice.md)、[docs/adr/0003-cube-walks-on-ground.md](docs/adr/0003-cube-walks-on-ground.md)、[docs/adr/0004-cube-walks-its-facing.md](docs/adr/0004-cube-walks-its-facing.md)、[docs/adr/0005-walk-along-camera-yaw.md](docs/adr/0005-walk-along-camera-yaw.md)、[docs/adr/0006-walk-stops-at-floor-edge.md](docs/adr/0006-walk-stops-at-floor-edge.md)、[docs/adr/0007-player-and-opponent-one-loop.md](docs/adr/0007-player-and-opponent-one-loop.md) にあります。

## 必要な環境

- Windows 10 以降
- Visual Studio 2022（「C++ によるデスクトップ開発」ワークロード）
- Windows 10 SDK
- CMake 3.24 以上
- Git（CMake の FetchContent が Dear ImGui を取得するため）

Dear ImGui は docking のタグ `v1.92.9b-docking` に固定しています。`shaders/lit.hlsl` はビルド時ではなく、起動時に `D3DCompile`（`d3dcompiler.lib`）で `vs_5_0` / `ps_5_0` にコンパイルします。ビルドすると、このファイルは実行ファイルの隣にコピーされます。

## 構成とビルド

Visual Studio 2022 の Developer PowerShell、または「x64 Native Tools Command Prompt」で、リポジトリのルートを開きます。

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

`Debug` も同じ手順です。`build\editor.sln` を Visual Studio 2022 で開いてもビルドできます。スタートアッププロジェクトは `editor` です。

## 実行

```bat
.\build\Release\editor.exe
```

Visual Studio から起動する場合、作業ディレクトリは実行ファイルのあるフォルダです。`lit.hlsl` をその場所から読みます。

| パネル | 内容 |
| --- | --- |
| Viewport | プレイヤーと相手の立方体、床。左ドラッグでカメラはプレイヤーの周りを回る。マウスがあるとき WASD でプレイヤーが歩く。床の端で止まる。相手は動かない |
| Player | プレイヤーの位置と回転（度） |
| Camera | プレイヤーからの距離。歩行をカメラのヨーに合わせる切り替え |
| Render | クリア色 |

## この環境では未検証

プレイヤーと相手を同じ並びにしたこの変更は、Windows SDK がない環境で書きました。ここではコンパイルも実行もしていません。
