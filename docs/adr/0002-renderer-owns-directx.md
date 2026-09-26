# DirectX は Renderer の中に留める

- 状態: `accepted`
- 日付: 2026-09-23
- 言葉: [../words.md](../words.md)

## なぜ

今の描画は `renderer.cpp` の DirectX 11 だけである。後から DirectX 12 へ移すときに、エンジン全体を書き直したくない。だからといって、今のうちに DirectX 12 を作る必要はない。

## 決めたこと

DirectX 11 のままにする。ハードウェアレイトレーシング、メッシュシェーダ、GPU 駆動の描画、コマンドの明示的な並列記録のどれかが実際に必要になるまで、DirectX 12 へは移さない。移すときは別の ADR で決める。

DirectX の型と呼び出しは `Renderer` の外へ出さない。ゲームプレイやパネルに `ID3D11*` は置かない。`SceneState` が持つのは、位置、回転、カメラ、クリア色のような、描画 API に依存しない値である。

HLSL は、頂点シェーダ、ピクセルシェーダ、定数バッファひとつに留める。

DirectX 11 の「今すぐ描く」形に合わせた共通インターフェースは足さない。その形にすると、後から DirectX 12 へ移しにくい。

ImGui の DirectX 11 バックエンドは、初期化と `RenderDrawData` だけなら `main.cpp` に残してよい。

## 採らない

今の DirectX 12 化。エフェクトフレームワーク。`Renderer` の外の `ID3D11DeviceContext`。デスクリプタヒープやバックエンド抽象の実装。

後から DirectX 12 へ移すときは、`renderer.cpp` と、ImGui の DirectX 11 バックエンドを呼んでいる行を書き直す。ウィンドウのループと `SceneState` は書き直さない。
