# editor

Windows 向けアクションゲームエンジンの、最初の画面です。実行ファイルはひとつだけで、名前は `editor` です。Win32 ウィンドウに DirectX 11 で、同じ立方体をプレイヤーと相手の二人分、その下の床、床の縁の壁と一緒に描きます。色は主体ごとに一色で、面ごとの色はありません。壁は別の一色です。Dear ImGui のドッキングでクリア色、プレイヤーの位置と回転、カメラ距離をその場で変えられます。ビューポート上の左ドラッグで、カメラはプレイヤーの周りを回ります。ビューポートにマウスがあるとき、WASD でプレイヤーが床の上を歩きます。相手は初期位置 `(0, 0, 4)` から動きません。既定の前後左右はプレイヤーのヨーで、歩行はヨーを変えません。向きは Player パネルの回転だけです。Camera パネルで切り替えると、前後左右はカメラの水平なヨーになり、移動中だけプレイヤーがその方向を向きます。動いたあと、壁か他の主体に重なった軸の移動だけを取り消します。壁の内面と相手の立方体の手前で止まります。Y は変えません。重力で落ちることはしません。ビューポートにマウスがあるとき、Space でプレイヤーが自分のヨーの前方を攻撃します。範囲は前面から前方へ 1.0、左右は中心から 0.5 の軸に沿った箱で、押した瞬間に一度だけ見ます。次の攻撃まで 0.4 秒空けます。各主体の残りは 3 から始まり、攻撃が届くと相手は 1 減ります。0 になると、その立方体は描かれず、歩行の障害にも攻撃の対象にもなりません。プレイヤーの残りは減りません。壁は攻撃を遮りません。歩きながらの接触では減りません。攻撃を出したフレームだけ、その判定と同じ箱を `(0.93, 0.82, 0.28)` の一色で描きます。当たっても空振りでも描きます。次のフレームには残りません。0.4 秒より短い連打では、箱は出ず、残りも減りません。

物理ライブラリ、アニメーション、相手の攻撃、勝敗の表示は含めていません。判断の記録は [docs/adr/0001-dx11-imgui-editor-slice.md](docs/adr/0001-dx11-imgui-editor-slice.md)、[docs/adr/0003-cube-walks-on-ground.md](docs/adr/0003-cube-walks-on-ground.md)、[docs/adr/0004-cube-walks-its-facing.md](docs/adr/0004-cube-walks-its-facing.md)、[docs/adr/0005-walk-along-camera-yaw.md](docs/adr/0005-walk-along-camera-yaw.md)、[docs/adr/0006-walk-stops-at-floor-edge.md](docs/adr/0006-walk-stops-at-floor-edge.md)、[docs/adr/0007-player-and-opponent-one-loop.md](docs/adr/0007-player-and-opponent-one-loop.md)、[docs/adr/0008-walk-stops-before-wall-and-subject.md](docs/adr/0008-walk-stops-before-wall-and-subject.md)、[docs/adr/0009-attack-reduces-remaining.md](docs/adr/0009-attack-reduces-remaining.md)、[docs/adr/0010-attack-box-one-frame.md](docs/adr/0010-attack-box-one-frame.md) にあります。

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
| Viewport | プレイヤーと相手の立方体、床、壁、各主体の残り。左ドラッグでカメラはプレイヤーの周りを回る。マウスがあるとき WASD でプレイヤーが歩き、Space で攻撃する。攻撃したフレームだけ判定の箱が出る。壁と相手の手前で止まる。相手の残りが 0 になると相手は消える。相手は動かない |
| Player | プレイヤーの位置と回転（度） |
| Camera | プレイヤーからの距離。歩行をカメラのヨーに合わせる切り替え |
| Render | クリア色 |

## この環境では未検証

攻撃したフレームに判定の箱を描くこの変更は、Windows SDK がない環境で書きました。ここではコンパイルも実行もしていません。
