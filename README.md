# editor

Windows 向けアクションゲームエンジンの、最初の画面です。実行ファイルはひとつだけで、名前は `editor` です。Win32 ウィンドウに DirectX 11 でライティングした立方体を描き、Dear ImGui のドッキングでクリア色、立方体の位置と回転、カメラ距離をその場で変えられます。ビューポート上の左ドラッグで、カメラは立方体の周りを回ります。

物理、アニメーション、戦闘など、ゲームプレイの仕組みはこのスライスに含めていません。判断の記録は [docs/adr/0001-dx11-imgui-editor-slice.md](docs/adr/0001-dx11-imgui-editor-slice.md) にあります。

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
| Viewport | 立方体。左ドラッグでカメラが立方体の周りを回る |
| Cube | 位置と回転（度） |
| Camera | 立方体からの距離 |
| Render | クリア色 |

## この環境では未検証

このツリーは Windows SDK も Visual Studio もない Linux 上で作成しました。ここではコンパイルも実行もしていません。Windows 上で上記の手順を通すまで、描画は確認できていません。
