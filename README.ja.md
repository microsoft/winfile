# ![icon](winfile.png) Windows ファイルマネージャー

Windows ファイルマネージャーは32ビット及び64ビット環境の現在のWindows(Windows 10も含む)で再び利用できる様になりました。

このリポジトリのmasterブランチには、以下の二つのタグが存在します。

1. original_plus: Windows NT4で利用されていた物から現在のWindowsで利用可能にするために最小限の変更が行われたバージョンです。
Visual Studioでコンパイルして実行する事ができます。

2. current master: 新機能が追加されたバージョンです。

不具合の報告や機能の提案をissuesに投稿またはプルリクエストを気軽に行ってください。

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
4. left and right arrows in the tree view expand and collapse folders like in the Explorer
5. added context menus in both panes
6. improved the means by which icons are displayed for files
7. F12 runs notepad or notepad++ on the selected file
8. moved the ini file location to %AppData%\Roaming\Microsoft\WinFile
9. File.Search can include a date which limits the files returned to those after the date provided;
the output is also sorted by the date instead of by the name
10. File.Search includes an option as to whether to include sub-directories
11. ctrl+K starts a command shell (ConEmu if installed) in the current directory; shift+ctrl+K
starts an elevated command shell (cmd.exe only)
12. File.Goto (ctrl+G) enables one to type a few words of a path and get a list of directories;
selecting one changes to that directory.  Only drive c: is indexed.
13. UI shows  reparse points (e.g., Junction points) as such
14. added simple forward / back navigation (probably needs to be improved)
15. View command has a new option to sort by date forward (oldest on top);
normal date sorting is newest on top

You can read the code for more details.

## 献上

### 貢献者とのライセンス契約
As mentioned above, this project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

### プルリクエストについて
献上や変更の提案に興味がある場合は、最初は[こちら](https://github.com/Microsoft/winfile/issues/88)を呼んでください。

## 利用規約

Copyright (c) Microsoft Corporation. All rights reserved.

Licensed under the [MIT](LICENSE) License.

このドキュメントは[@Takym](https://github.com/Takym)により翻訳されました。
