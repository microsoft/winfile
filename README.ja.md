# ![icon](winfile.png) Windows ファイルマネージャー

Windows ファイルマネージャーは32ビット及び64ビット環境の現在のWindows(Windows 10も含む)で再び利用できる様になりました。

このリポジトリのmasterブランチには、以下の二つのタグが存在します。

1. original_plus: Windows NT4で利用されていた物から現在のWindowsで利用可能にするために最小限の変更が行われたバージョンです。
Visual Studioでコンパイルして実行する事ができます。

2. current master: 新機能が追加されたバージョンです。

不具合の報告や機能の提案をissuesに投稿またはプルリクエストは気軽に行ってください。

original_plusブランチのソースコードは一切変更しません。また、ご自由にソースコードを利用してください。

## ダウンロード
ソースコードではなく実行可能バイナリを利用したい場合は、以下のリンクからダウンロードしてください。

masterブランチからの最新版は現在は準備中です。

[最新安定板 (v10.0)](https://github.com/Microsoft/winfile/releases/tag/v10.0)

[Original_Plus](https://github.com/Microsoft/winfile/releases/tag/original_plus)

その他のリリースは[Releases](https://github.com/Microsoft/winfile/releases)ページをご覧ください。


## 歴史

初代Windows ファイルマネージャーはWindows 3.0と共に1990年代初めにリリースされました。
詳しくは[https://en.wikipedia.org/wiki/File_Manager_(Windows)](https://en.wikipedia.org/wiki/File_Manager_(Windows))をご覧ください。

## スクリーンショット

![(https://commons.wikimedia.org/wiki/File:Winfile-v10-0-file-manager_%28cropped%29.png)](https://upload.wikimedia.org/wikipedia/commons/6/67/Winfile-v10-0-file-manager_%28cropped%29.png)

[@Speps](https://github.com/speps)さんリンクを提供して頂き誠にありがとうございます。
誰がこの画像をWikiMediaにアップロードしたのかは分かりません。

## original_plus での変更点

srcディレクトリ内のソースコードは2007年11月のWindows NT 4のソースツリーからコピーされました。
original_plusタグは最新のWindowsでWinFile.exeを実行できる様に変更した物です。それ以外の変更は制限されています。

主な重要な変更は次の通りです：

1. Visual Studio 2015 または 2017 で利用できる形に変換されました。
2. 64ビットWindowsで実行可能になりました。(例: GetWindowLong -> GetWindowLongPtr, LONG -> LPARAM)
3. NTのソースツリーの別の場所で定義されていたヘッダファイルを追加しました。(例: wfext.h)
4. 利用されていないファイルを削除しました。(例: winfile.def)
5. 64ビットの演算処理を内部ライブラリからCに変換しました。
6. 内部シェルAPIを公開APIに変換しました。(古いバージョンの物は動作しない為です。)

helpディレクトリにはwinfile.hlpとwinfile.chmの両方が入っています。
winfile.hlpはNT4に入っていた物ですがWindows 10では利用できません。
winfile.chmはWindows 98からコピーされました。こちらはWindows 10から利用する事ができます。
ただし、WinFile.exeはwinfile.hlpを起動しようとして失敗します。

ローカルブランチを作成するには、`git checkout -b <your branch> original_plus`を実行してください。

## original_plus後のmaster v10.0の変更点

masterブランチには2007年以降の変更が含まれます。これは単に私の個人的な変更です。
幾つかの変更は私がこのツールを使う上で必要な物だけに制限されています。
例えば、新たにgotoコマンドをサポートしますがCドライブに関する情報しか保持していません。

このWinFileは大きく設計が変更されていません。

バージョン10.0は2007年11月からこのOSSプロジェクトが作成されるまでの変更全体を含みます。
v10.0の変更はコミット履歴とリリースをご覧ください。

v10.0では以下の変更があります：

1. OLEのドラッグ＆ドロップをサポートしました。
2. コントロールキー(例: Ctrl+C)をドライブを変更する代わりに現在のショートカットキー(例: Ctrl+C -> コピー)へ対応しました。
3. 切り取り(Ctrl+X)と貼り付け(Ctrl+V)はファイルの移動を表します。
4. 左右キーを利用してエクスプローラの様にツリービューでフォルダを展開または折りたたむ事ができる様になりました。
5. コンテキストメニューを追加しました。
6. ファイルアイコンの表示が向上しました。
7. F12キーで選択しているファイルをメモ帳で開ける様にしました。
8. INIファイルを %AppData%\Roaming\Microsoft\WinFile に移動しました。
9. ファイル検索で日付を利用してファイルを絞り込む事ができる様になりました。
その場合、ファイルは日付順で並び替えられます。
10. ファイル検索で子ディレクトリを含むかどうかを変更できる様になりました。
11. Ctrl+Kキーを現在のディレクトリでコマンドシェル(ConEmuがインストールされている場合、そちらが起動します)を開く事ができます。
Shift+Ctrl+Kで管理者権限でコマンドシェルが起動します。(コマンドプロンプトのみ)
12. 「別のディレクトリへ移動」(Ctrl+G)で数単語だけ入力するとディレクトリの一覧が表示されます。
その中から一つ選択して移動する事もできます。Cドライブのみインデックスされています。
13. UIは解析ポイント(例: ジャンクション)をそのまま表示します。
14. 簡易的な履歴の戻る/進む処理が追加されました。(改善中です。)
15. 日付順で並び替えができる様になりました。

これ以外の変更はソースコードを参照してください。

(ここから下はライセンス関連の項目です。敢えて翻訳せず原文のままにして置きました。)

## Contributing

### Contributor License Agreement
As mentioned above, this project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

### What Makes a Good Pull Request for WinFile?
If you are interested in contributing and/or suggesting changes to the actual application, you might find it helpful to [read this post first](https://github.com/Microsoft/winfile/issues/88).

## License

Copyright (c) Microsoft Corporation. All rights reserved.

Licensed under the [MIT](LICENSE) License.

This document is translated by [@Takym](https://github.com/Takym).
