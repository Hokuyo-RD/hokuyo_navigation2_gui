# CSS の構成

GUI のスタイルは「値 → 素の要素 → 共有部品 → 画面固有」の 4 層に分けている。
どの画面でも見た目が揃うように、**新しいスタイルを書く前に、まず既存の部品で
表現できないかを確認する**こと。

```
static/css/
├── tokens.css        1. 値      色・余白・角丸・影・フォント（CSS変数）
├── base.css          2. 要素    body / h1 / a / input など素のHTML要素
├── components.css    3. 部品    .btn / .container / .file-list / .alert …
├── main.css          4. 入口    上3つを @import ＋ アプリシェル（layout.html用）
└── pages/            5. 画面固有  その画面でしか使わないものだけ
    ├── map-viewer.css
    ├── file-browser.css
    ├── rosbag-topics.css
    └── editor.css
```

## 使い方

すべてのテンプレートは `main.css` を 1 枚だけ読み込む。

```html
<link rel="stylesheet" href="{{ url_for('static', filename='css/main.css') }}">
```

画面固有のスタイルがあるときだけ、続けて `pages/*.css` を読み込む。

```html
<link rel="stylesheet" href="{{ url_for('static', filename='css/pages/map-viewer.css') }}">
```

## ルール

1. **生の値を書かない。** 色や余白は必ず `var(--color-primary)` のように
   `tokens.css` の変数を使う。色を変えたいときは `tokens.css` だけを直せばよい。
2. **テンプレートに `<style>` を書かない。** 使い回すものは `components.css`、
   その画面だけのものは `pages/*.css` に置く。
3. **`style="..."` 属性を増やさない。** レイアウトの微調整も含めてクラスで表現する。
4. **`!important` を使わない。** 効かないときは詳細度（セレクタの組み立て）を
   見直す。唯一の例外は `base.css` の `[hidden]`（要素を確実に隠すため）。
5. **命名は BEM 風**に統一する。
   - ブロック: `.file-list`
   - 要素: `.file-list__name`
   - 修飾子: `.btn--danger`
   - 状態（JavaScript が付け外しするもの）: `.selected` / `.is-disabled`

## 主な部品

| 部品 | クラス | 用途 |
| --- | --- | --- |
| カード | `.container`（広い表は `.container--wide`） | 1枚もの画面の白いパネル |
| ボタン | `.btn` ＋ `.btn--primary/success/danger/secondary/warning` | 全画面共通のボタン。大きさは `.btn--sm` / `.btn--lg` / `.btn--block` |
| ボタンの並び | `.actions`（右寄せ `.actions--end`） | 画面下部の操作ボタン群 |
| 選択リスト | `.file-list`（`.file-list--selectable`） | ファイル一覧。選択中は `.selected` |
| 情報リスト | `.info-list` / `.info-list__item` | 選択済みファイルなど読むだけの一覧 |
| 通知 | `.alert` ＋ `.alert--success/error/warning/info` | flash メッセージ。修飾子は Flask の flash カテゴリと同名 |
| 進捗 | `.progress-panel` / `.spinner` | 変換・マッピングの状態表示 |
| 指摘 | `.issue-list` / `.issue` ＋ `.alert--*` | 設定チェック結果・実行ログから読み取った問題（見出し／状況／対処の3段） |
| 折りたたみ | `.issue-collapse`（`<details>`） | 「確認できた項目」など、既定で畳んでおく一覧 |
| 実行ログ | `.log-console` / `.log-console__body` | 別ターミナルで動く処理の出力をそのまま表示 |
| 設定編集表 | `.config-table`（`.table` と併用） | パラメータ設定CSVをその場で書き換える表 |
| 表 | `.table` | CSV 編集など |
| トグル | `.switch` / `.switch__slider` / `.switch-row` | オン・オフ切替 |
| オーバーレイ | `.popup-container` / `.popup-content` / `.modal` | ダイアログ |

## アプリシェルと 1 枚もの画面

- `layout.html` を継承する画面（メニュー・実行結果・停止）は
  `<body class="app-shell">`。この中のボタンは、タッチ操作用の大きな
  メニューボタンの見た目になる（`main.css`）。
- それ以外の 1 枚もの画面は `<body class="page">` で、中身を `.container` で
  包み、ボタンには `.btn` を使う。
