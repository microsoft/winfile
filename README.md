# ![icon](winfile.png) Windows File Manager (WinFile)

The Windows File Manager lives again and runs as a native x86 and x64 desktop app
on all currently supported version of Windows, including Windows 10 & 11. I welcome your thoughts, comments and suggestions.

This current master was forked off from [microsoft/winfile](https://github.com/microsoft/winfile) and contains changes/additions to WinFile over the years by many contributors. Furthermore it contains additions which I think are usefull or bug fixes.

I will consider bugs fixes and suggestions for minor changes to the master branch. Feel free to create a pull request or post issues as you see fit.

## Download The App
If you just want to download the WinFile application without worrying about compiling from the source code, go for the pre-compiled version available.

- [Latest Release on Github (v10.2.1.0)](https://github.com/schinagl/winfile/releases/tag/v10.2.1.0)

To see older versions, [see the releases page](https://github.com/Microsoft/winfile/releases).


## History
The Windows File manager was originally released with Windows 3.0 in the early 1990s.  You
can read more about the history at https://en.wikipedia.org/wiki/File_Manager_(Windows).

## What it looks like
# ![Winfile](winfilescreenshot.png)

## Changes in master v10.2.1.0
In summary, v10.2.1.0 has the following changes/new features:

1. OLE drag/drop support
2. control characters (e.g., ctrl+C) map to current shortcut (e.g., `ctrl+C` -> `copy` and copy path to clipboard)
instead of changing drives. Change drives now via CTRL + ALT + letter
3. cut (`ctrl+X`) followed by paste (`ctrl+V`) translates into a file move as one would expect
4. left and right arrows in the tree view expand and collapse folders like in the Explorer
5. added context menus in both panes
6. improved the means by which icons are displayed for files
7. F12 runs notepad or notepad++ on the selected file. Configure via Winfile.ini[Settings]EditorPath
8. moved the ini file location to `%AppData%\Roaming\Microsoft\WinFile`
9. File.Search can include a date which limits the files returned to those after the date provided;
the output is also sorted by the date instead of by the name
10. File.Search includes an option as to whether to include sub-directories
11. `ctrl+K` starts a command shell (ConEmu if installed) in the current directory; `shift+ctrl+K`
starts an elevated command shell (`cmd.exe` only)
12. File.Goto (`ctrl+G`) enables one to type a few words of a path and get a list of directories;
selecting one changes to that directory.  Default = c:\\. Configure via Winfile.ini[Settings]CachedPath
13. UI shows reparse points (e.g., Junction Points and Symbolic Links) as such with little icons.
14. added simple forward / back navigation (probably needs to be improved)
15. View command has a new option to sort by date forward (oldest on top); normal date sorting is newest on top; Icon in tool-bar available
16. CTRL + ENTER executes associated files as administrator
17. Symbolic Link directories/files and can be created by pressing CTRL + SHIFT during drag and drop of directories/files
18. Hardlinks and Junctions and can be created by pressing CTRL + SHIFT + ALT during drag and drop of directories/files
19. The delimiters for splitting words for the GotoCache can be configured via Winfile.ini[Settings]GotoCachePunctuation. The default is - _.
20. The scroll behavior of the treeview on expand can be configured via Winfile.ini[Settings]ScrollOnExpand. The default is to scroll.
21. Can handle paths up to 1024 characters with Windows10 >= 1607. Set HKLM\SYSTEM\CurrentControlSet\Control\FileSystem\LongPathsEnabled=1 as admin.
22. Japanese localisation with full-width katakanas
23. Create files with suffix '- Copy', when copying with (`ctrl+C`) -> (`ctrl+V`) in the same dir, or drag-copy with mouse onto empty space in same dir.
24. UNC Path support via CTRL-G. Shows up as 'digit drive' 0: - 9:. Select it either via CTRL-0 ... CTRL-9 or from drive drop-down. Remove UNC digit drive with CTRL-W

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

## License
Copyright (c) Microsoft Corporation. All rights reserved.

Licensed under the [MIT](LICENSE) License.
