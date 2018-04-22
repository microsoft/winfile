# ![icon](winfile.png) Windows File Manager (WinFile)

The Windows File Manager lives again and runs as a native x86 and x64 desktop app
on all currently supported version of Windows, including Windows 10. I welcome your thoughts, comments and suggestions.

There are two primary versions of the source code in the master branch:

1. original_plus tag: refers to the source for WinFile as of Windows NT4 with minimal changes
so that it compiles with Visual Studio and runs on current Windows.

2. current master: contains my personal changes / additions to WinFile.

I will consider bugs fixes and suggestions for minor changes to the master branch.  Feel free to create a pull request or post issues as you see fit.

I will not be changing the original_plus sources nor creating other branches for other purposes.
You are welcome do that on your own.

## Download The App
If you just want to download the WinFile application without worrying about compiling from the source code, we have precompiled versions available for both original_plus as well as the latest stable release for you to download. Source code for the corresponding release is also, optionally, available.

Please select the version which you would like to download.

[Latest Stable Release (v10.0)](https://github.com/Microsoft/winfile/releases/tag/v10.0)

[Original_Plus](https://github.com/Microsoft/winfile/releases/tag/original_plus)

To see more release binaries, including of older versions, [see the releases page](https://github.com/Microsoft/winfile/releases).


## History

The Windows File manager was originally released with Windows 3.0 in the early 1990s.  You
can read more about the history at https://en.wikipedia.org/wiki/File_Manager_(Windows).

## What it looks like

![(https://commons.wikimedia.org/wiki/File:Winfile-v10-0-file-manager_%28cropped%29.png)](https://upload.wikimedia.org/wikipedia/commons/6/67/Winfile-v10-0-file-manager_%28cropped%29.png)

Thanks to [@Speps](https://github.com/speps) for the link; not sure who uploaded the image to Wikimedia.

## Changes in original_plus

The source code provided here (in the src directory) was copied from the Windows NT 4 source tree in November
2007.  The tag named original_plus contains a very limited set of  modifications
from the original sources to enable WinFile.exe to run on current Windows.
The most significant changes are:

1. converted to Visual Studio solution; works on VS 2015 and 2017
2. compiles and runs on 64-bit Windows (e.g., GetWindowLong -> GetWindowLongPtr, LONG -> LPARAM)
3. added a few header files which were stored elsewhere in the NT source tree (e.g., wfext.h)
4. deleted some unused files (e.g., winfile.def)
5. converted 64-bit arithmetic from internal libraries to C
6. converted internal shell APIs to public APIs (the primary reason the old version would not run)

The help directory contains both winfile.hlp and winfile.chm.  Winfile.hlp was in the NT4
source tree, but does not work on Windows 10 any more.  Winfile.chm was copied from 
a regular installation of Windows 98 and works on Windows 10.  As is, WinFile.exe 
tries to launch winfile.hlp which fails.

To create your own local branch referring to this release, run "git checkout -b <your branch> original_plus".

## Changes in master v10.0 after original_plus

The master branch contains changes I have made since 2007.  The changes have been solely determined
by my needs and personal use.  Some of the changes have limitations that fit the way I use the tool.
For example, the path index which supports the new goto command only contains information for the c: drive.

I have also not redesigned or restructured WinFile in any major way.

Version v10.0 represents the entire set of changes from Nov. 2007 until this OSS project
was created.  For changes post v10.0, see the commit and release history.

In summary v10.0 has the following changes/new features compared to original_plus:

1. OLE drag/drop support
2. control characters (e.g., ctrl+C) map to current short cut (e.g., ctrl+c -> copy)
instead of changing drives
3. cut (ctrl+X) followed by paste (ctrl+V) translates into a file move as one would expect
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
