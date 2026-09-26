# editor

Windows 向けアクションゲームエンジンの、最初の画面です。実行ファイルはひとつだけで、名前は `editor` です。Win32 ウィンドウに DirectX 11 で、同じ立方体をプレイヤーと相手、その下の床、床の縁の壁と一緒に描きます。起動時の相手は一人です。パネルから 0 人から 3 人まで増減できます。色は主体ごとに一色で、面ごとの色はありません。壁は別の一色です。Dear ImGui のドッキングでクリア色、各主体の位置、先頭の回転、それ以外の主体のヨー、床の半辺、カメラ距離をその場で変えられます。床の半辺は 4 から 40 で、壁はその縁に付きます。起動したときは編集で、時間は止まっています。Subjects の Start を押したときだけ試走が始まり、プレイヤーと相手の歩行と攻撃が進みます。試走中は位置、ヨー、相手の増減、床の半辺を出さず、Scene メニューは無効です。編集中は、Scene メニューで名前を付けてシーンを複数保存し、一覧から選ぶと配置を読み込みます。一覧から読んだあと、さらに編集して Start を押すと、そのときの配置を開始状態として覚えます。保存していない変更も入ります。残りは 3、クールダウンは 0 から始めます。戦闘の途中経過は覚えません。一つのシーンは作業ディレクトリの `scenes` にあり、`floor` と床の半辺、主体の位置、ヨー、色だけを書きます。残りとクールダウンは保存しません。読み込んだ主体の残りは 3、クールダウンは 0、ピッチとロールは 0 です。壊れたファイル、数が範囲外、主体が 0 人または 5 人以上のときは配置を変えず、Subjects に失敗を出します。起動時はコードの初期配置で、`arena.txt` は読まず、既にあるファイルも消しません。保存しても、コードの 1 対 1 の初期配置は上書きしません。編集中、Scene メニューの Reset to initial は、今の配置をその 1 対 1 に戻します。プレイヤーは `(0, 0, 0)`、ヨー 0 度、色 `(0.78, 0.48, 0.27)`、相手は `(0, 0, 4)`、ヨー 180 度、色 `(0.25, 0.42, 0.68)`、床の半辺は 20 です。この距離では攻撃の箱は届きません。保存してあるシーンは消えません。試走中はこの操作を出しません。ビューポート上の左ドラッグで、カメラはプレイヤーの周りを回ります。試走中、ビューポートにマウスがあるとき、WASD でプレイヤーが床の上を歩きます。相手は初期位置 `(0, 0, 4)` から、プレイヤーの XZ へ秒あたり 2.5 で歩きます。移動中のヨーはその方向を向きます。攻撃の箱がプレイヤーに重なる距離では立ち止まり、重なってから 0.5 秒待って最初の攻撃を出します。範囲を外れると待ちは消え、次に入ったときまた 0.5 秒待ちます。居続けるあいだは 0.8 秒に一度出します。既定の前後左右はプレイヤーのヨーで、歩行はヨーを変えません。向きは Subjects パネルの回転だけです。相手のヨーをドラッグしているあいだは、その向きのまま歩きます。Camera パネルで切り替えると、前後左右はカメラの水平なヨーになり、移動中だけプレイヤーがその方向を向きます。動いたあと、壁か他の主体に重なった軸の移動だけを取り消します。相手の歩行も同じです。壁の内面と相手の立方体の手前で止まります。Y は変えません。重力で落ちることはしません。ビューポートにマウスがあるとき、Space でプレイヤーが自分のヨーの前方を攻撃します。範囲は前面から前方へ 1.0、左右は中心から 0.5 の軸に沿った箱で、押した瞬間に一度だけ見ます。次の攻撃まで 0.4 秒空けます。クールダウンの最後の 0.15 秒に押した Space は覚え、0 になったフレームに一度出します。それより前の Space は捨てます。各主体の残りは 3 から始まり、攻撃が届くと相手は 1 減ります。相手の攻撃が届くとプレイヤーも 1 減ります。0 になると、その立方体は描かれず、歩行の障害にも攻撃の対象にもなりません。プレイヤーの残りが 0 になると試走が止まります。使っている相手が 1 人以上いて全員の残りが 0 のときも止まります。相手が 0 人のときは、相手の全滅では止まりません。止めても配置は自動では戻りません。Return to start で開始した配置に戻り、編集に戻ります。試走は自動では再開しません。一部の相手だけが 0 のときは、その相手は止まり、試走は続き、プレイヤーは歩けます。壁は攻撃を遮りません。歩きながらの接触では減りません。攻撃を出したフレームから 3 フレーム、その判定と同じ箱を `(0.93, 0.82, 0.28)` の一色で描きます。出始めと終わりは 0 フレームです。当たっても空振りでも描きます。同じ箱は一つの主体の残りを一度だけ減らします。試走が止まっても、その箱は残フレームのあいだ残ります。編集に戻ると消えます。当たった主体は、次のフレームから 4 フレーム、歩きと攻撃の時間が 0 になります。ほかの主体は止まりません。箱の寿命は止まりません。同じフレームに両方が出した箱は両方残します。0.4 秒より短い間隔では新しい箱は出ず、残りも減りません。最後の 0.15 秒に覚えた Space は、クールダウンが 0 になったフレームに箱を出します。

物理ライブラリとアニメーションは含めていません。全滅のあとは試走を止め、戻す操作を出します。勝敗の文言は出しません。判断の記録は [docs/adr/0001-dx11-imgui-editor-slice.md](docs/adr/0001-dx11-imgui-editor-slice.md)、[docs/adr/0003-cube-walks-on-ground.md](docs/adr/0003-cube-walks-on-ground.md)、[docs/adr/0004-cube-walks-its-facing.md](docs/adr/0004-cube-walks-its-facing.md)、[docs/adr/0005-walk-along-camera-yaw.md](docs/adr/0005-walk-along-camera-yaw.md)、[docs/adr/0006-walk-stops-at-floor-edge.md](docs/adr/0006-walk-stops-at-floor-edge.md)、[docs/adr/0007-player-and-opponent-one-loop.md](docs/adr/0007-player-and-opponent-one-loop.md)、[docs/adr/0008-walk-stops-before-wall-and-subject.md](docs/adr/0008-walk-stops-before-wall-and-subject.md)、[docs/adr/0009-attack-reduces-remaining.md](docs/adr/0009-attack-reduces-remaining.md)、[docs/adr/0010-attack-box-one-frame.md](docs/adr/0010-attack-box-one-frame.md)、[docs/adr/0011-opponent-same-attack.md](docs/adr/0011-opponent-same-attack.md)、[docs/adr/0012-edit-subject-pose.md](docs/adr/0012-edit-subject-pose.md)、[docs/adr/0013-add-remove-opponents.md](docs/adr/0013-add-remove-opponents.md)、[docs/adr/0014-save-arena-layout.md](docs/adr/0014-save-arena-layout.md)、[docs/adr/0015-edit-then-start-a-trial.md](docs/adr/0015-edit-then-start-a-trial.md)、[docs/adr/0016-winnable-trial-attack.md](docs/adr/0016-winnable-trial-attack.md)、[docs/adr/0017-actions-reach-subjects.md](docs/adr/0017-actions-reach-subjects.md)、[docs/adr/0018-opponent-state-is-actions.md](docs/adr/0018-opponent-state-is-actions.md)、[docs/adr/0019-attack-volume-lifetime.md](docs/adr/0019-attack-volume-lifetime.md)、[docs/adr/0020-subject-frame-dt.md](docs/adr/0020-subject-frame-dt.md) にあります。

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

Visual Studio から起動する場合、作業ディレクトリは実行ファイルのあるフォルダです。`lit.hlsl` をその場所から読み、シーンは `scenes` に保存します。既にある `arena.txt` は読まず、消しません。

| 場所 | 内容 |
| --- | --- |
| Scene | 画面上部のメニュー。編集中だけ有効。シーン名、Save scene、一覧、Reset to initial。一覧を選ぶとその配置を読む。初期状態に戻すと、攻撃の届かない 1 対 1 になり、保存したシーンは残る。試走中と、止めたあとは無効 |
| Viewport | プレイヤーと相手の立方体、床、壁、各主体の残り。左ドラッグでカメラはプレイヤーの周りを回る。試走中、マウスがあるとき WASD でプレイヤーが歩き、Space で攻撃する。相手は近づき、間合いに入って 0.5 秒後に攻撃し、その後は 0.8 秒ごとです。プレイヤーは 0.4 秒ごとで、終わり 0.15 秒の Space を覚えます。攻撃したフレームから 3 フレーム、判定の箱が出る。同じ箱は一度だけ当たる。壁と主体の手前で止まる。残りが 0 になると、その立方体は消える。編集中は歩かない |
| Subjects | 編集中は各主体の位置。先頭はピッチ、ヨー、ロール。それ以外はヨーだけ。相手の追加と、末尾の削除。最大 4 人。床の半辺、Start。読み込みや保存の失敗は、ここに出す。試走中は配置の編集を出さない。全滅で止めたあとは Return to start だけを出す |
| Camera | プレイヤーからの距離。歩行をカメラのヨーに合わせる切り替え |
| Render | クリア色 |

## この環境では未検証

先行入力と相手の攻撃間隔を分けるこの変更は、Windows SDK がない環境で書きました。ここではコンパイルも実行もしていません。
