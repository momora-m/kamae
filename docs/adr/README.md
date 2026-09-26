# Architecture Decision Records

後から覆しにくい判断を短く残す。実装の手順、画面の説明、日々の修正は書かない。

言葉は [../words.md](../words.md)。各 ADR は一つの判断、選んだ理由、採らないものだけを書く。隣の ADR は繰り返さない。

## ファイル名

`NNNN-kebab-title.md`

- `NNNN` は 4 桁の連番。`0001` から始め、一度使った番号は再利用しない。
- `kebab-title` は判断が分かる短い英語。単語はハイフンでつなぐ。

例: `0001-dx11-imgui-editor-slice.md`

## 状態

各 ADR の先頭に、次のいずれかひとつを書く。

| 状態 | 意味 |
| --- | --- |
| `proposed` | まだ採用していない提案 |
| `accepted` | 採用し、実装の前提にしている |
| `superseded` | 別の ADR に置き換わった。新しい文書へのリンクを残す |

`superseded` にするときは本文を消さず、置き換える ADR を新しく足す。一部だけ後から変わったときは、状態は `accepted` のまま、変わった箇所を本文に書く。

## 一覧

| ADR | 判断 |
| --- | --- |
| [0001](0001-dx11-imgui-editor-slice.md) | 最初の画面は DirectX 11 と Dear ImGui |
| [0002](0002-renderer-owns-directx.md) | DirectX は `Renderer` の中 |
| [0003](0003-cube-walks-on-ground.md) | 床の上を歩く。向きは 0004 が置き換えた |
| [0004](0004-cube-walks-its-facing.md) | 既定は自分のヨーで歩く |
| [0005](0005-walk-along-camera-yaw.md) | 切り替えるとカメラの水平ヨーで歩く |
| [0006](0006-walk-stops-at-floor-edge.md) | 床から落ちない。端の固定は 0008 が置き換えた |
| [0007](0007-player-and-opponent-one-loop.md) | プレイヤーと相手は同じ並び |
| [0008](0008-walk-stops-before-wall-and-subject.md) | 壁と他のキャラクターの手前で止まる |
| [0009](0009-attack-reduces-remaining.md) | 攻撃が届くと体力が減る |
| [0010](0010-attack-box-one-frame.md) | 判定と同じヒットボックスを描く。1 フレームだけは 0019 が置き換えた |
| [0011](0011-opponent-same-attack.md) | 相手は同じ攻撃でプレイヤーを倒す |
| [0012](0012-edit-subject-pose.md) | パネルで位置とヨーを編集する |
| [0013](0013-add-remove-opponents.md) | 相手は 0 人から 3 人 |
| [0014](0014-save-arena-layout.md) | 配置ファイルの行。`arena.txt` 一つと起動時読みは 0015 が置き換えた |
| [0015](0015-edit-then-start-a-trial.md) | 編集モードとプレイモード。名前付きシーンから始める |
| [0016](0016-winnable-trial-attack.md) | プレイヤーが先に当てられる間隔 |
| [0017](0017-actions-reach-subjects.md) | キャラクターが受け取るのは歩きと攻撃する／しない |
| [0018](0018-opponent-state-is-actions.md) | 相手の状態は、その入力の組み合わせ |
| [0019](0019-attack-volume-lifetime.md) | ヒットボックスは 3 フレーム |
| [0020](0020-subject-frame-dt.md) | キャラクターごとに経過秒。被弾は 4 フレーム 0 |
| [0021](0021-controller-attaches-to-subject.md) | コントローラーはキャラクターに付く |
| [0022](0022-session-owns-volumes.md) | ヒットボックスはプレイが持つ並び |
| [0023](0023-hit-is-session-result.md) | ヒットはプレイが適用する結果 |
