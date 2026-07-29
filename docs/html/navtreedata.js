/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "ChArUco Board を使ったカメラキャリブレーション", "index.html", [
    [ "ゲームグラフィックス特論の宿題用補助プログラム GLFW3 版.", "index.html", null ],
    [ "作業指示および開発履歴", "md_REQUESTS.html", [
      [ "概要", "md_REQUESTS.html#autotoc_md1", null ],
      [ "作業履歴", "md_REQUESTS.html#autotoc_md2", [
        [ "1. OpenCV依存の排除とMedia Foundation化", "md_REQUESTS.html#autotoc_md3", null ],
        [ "2. MFTによるH.264手動デコード実装", "md_REQUESTS.html#autotoc_md4", null ],
        [ "3. 初期化遅延（フリーズ）の回避（Lazy Initialization）", "md_REQUESTS.html#autotoc_md5", null ],
        [ "4. MFT バッファ管理と低遅延（Low Latency）化", "md_REQUESTS.html#autotoc_md6", null ],
        [ "5. レイテンシ優先モード（Drop old frames）の導入と例外修正", "md_REQUESTS.html#autotoc_md7", null ]
      ] ],
      [ "現在のステータスと課題", "md_REQUESTS.html#autotoc_md8", [
        [ "6. コードのクリーンアップと堅牢性 (Robustness) の向上", "md_REQUESTS.html#autotoc_md9", null ],
        [ "7. 依存ライブラリ管理とビルド環境のアップデート", "md_REQUESTS.html#autotoc_md10", null ],
        [ "8. キャプチャ開始時に投影方式固有のパラメータが失われる問題の修正", "md_REQUESTS.html#autotoc_md11", null ],
        [ "9. <span class=\"tt\">Menu</span>を中心とした状態管理とクラス境界の整理", "md_REQUESTS.html#autotoc_md12", null ],
        [ "10. GStreamer対応の廃止", "md_REQUESTS.html#autotoc_md13", null ],
        [ "11. 入力画像の表示領域への自動フィット", "md_REQUESTS.html#autotoc_md14", null ],
        [ "12. 入力オープン時の初期画角計算の復元", "md_REQUESTS.html#autotoc_md15", null ],
        [ "13. クラスメンバ変数の初期化位置の最適化", "md_REQUESTS.html#autotoc_md16", null ],
        [ "14. 共通処理における命名規約・コメントの統一とドキュメント同期", "md_REQUESTS.html#autotoc_md17", null ],
        [ "15. mfcapture における GStreamer 関連コードの削除", "md_REQUESTS.html#autotoc_md18", null ]
      ] ]
    ] ],
    [ "名前空間", "namespaces.html", [
      [ "名前空間一覧", "namespaces.html", "namespaces_dup" ],
      [ "名前空間メンバ", "namespacemembers.html", [
        [ "全て", "namespacemembers.html", null ],
        [ "関数", "namespacemembers_func.html", null ],
        [ "変数", "namespacemembers_vars.html", null ],
        [ "列挙型", "namespacemembers_enum.html", null ],
        [ "列挙値", "namespacemembers_eval.html", null ]
      ] ]
    ] ],
    [ "クラス", "annotated.html", [
      [ "クラス一覧", "annotated.html", "annotated_dup" ],
      [ "クラス索引", "classes.html", null ],
      [ "クラス階層", "hierarchy.html", "hierarchy" ],
      [ "クラスメンバ", "functions.html", [
        [ "全て", "functions.html", "functions_dup" ],
        [ "関数", "functions_func.html", "functions_func" ],
        [ "変数", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "ファイル", "files.html", [
      [ "ファイル一覧", "files.html", "files_dup" ],
      [ "ファイルメンバ", "globals.html", [
        [ "全て", "globals.html", null ],
        [ "関数", "globals_func.html", null ],
        [ "型定義", "globals_type.html", null ],
        [ "マクロ定義", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"Buffer_8cpp.html",
"classFramebuffer.html#a34a26dfb3f1b50c3ae503ee74fb5d9d5",
"classgg_1_1GgMatrix.html#a2455429beaf82954d49fa3ca25143177",
"classgg_1_1GgQuaternion.html#a345f63c1d1278daece7b1a09d279d74a",
"classgg_1_1GgSimpleShader.html#a3b01a2498e26aad446c5bb58271d3f51",
"classgg_1_1GgUniformBuffer.html#ad35bb060eb37e5f3679e24f469016eeb",
"namespacegg.html#aa3a225df6ac13de39d971cb22ee8f46e"
];

var SYNCONMSG = 'クリックで同期表示が無効になります';
var SYNCOFFMSG = 'クリックで同期表示が有効になります';
var LISTOFALLMEMBERS = '全メンバ一覧';