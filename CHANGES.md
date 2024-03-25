
## Changes in master v10.0 after original_plus

The master branch contains changes made since 2007.  WinFile has not been redesigned or restructured in any significant way.

Version v10.0 represents the entire set of changes from Nov. 2007 until this OSS project
was created.  For changes post v10.0, summaries are provided below, but the complete
commit history is also available.

## Changes in v10.3 compared to v10.2 (March 2024)

1. Ability to launch Administrator command prompt or Explorer in a directory
2. More entries in the Goto dialog, and ability to Page Up/Page Down through the list
3. Resizable dialogs throughout
4. Symbolic links are visually distinguished using &gt;SYMLINK&lt;
5. Translation improvements
6. Can launch the Store version from the command line
7. Refresh is more thorough
8. Can specify an initial directory when opening Winfile
9. Source and tooling improvements, including an ability to target Windows XP

## Changes in v10.2 compared to v10.0 (December 2022)

1. Editor configurable via Winfile.ini[Settings]EditorPath
2. Change drives now via CTRL + ALT + letter
3. File.Goto paths can be configured.  Default = C:\\. Configure via Winfile.ini[Settings]CachedPath
4. `ctrl+C` copies path text to clipboard
5. CTRL + ENTER executes associated files as administrator
6. Symbolic Link directories/files and can be created by pressing CTRL + SHIFT during drag and drop of directories/files
7. Hardlinks and Junctions and can be created by pressing CTRL + SHIFT + ALT during drag and drop of directories/files
8. The delimiters for splitting words for the GotoCache can be configured via Winfile.ini[Settings]GotoCachePunctuation. The default is - _.
9. The scroll behavior of the treeview on expand can be configured via Winfile.ini[Settings]ScrollOnExpand. The default is to scroll.
10. Can handle paths up to 1024 characters with Windows10 >= 1607. Set HKLM\SYSTEM\CurrentControlSet\Control\FileSystem\LongPathsEnabled=1 as admin.
11. Japanese localisation with full-width katakanas
12. Create files with suffix '- Copy', when copying with (`ctrl+C`) -> (`ctrl+V`) in the same dir, or drag-copy with mouse onto empty space in same dir.
13. AMD64 build available

## Changes in v10.0 compared to original_plus (April 2018)

1. OLE drag/drop support
2. control characters (e.g., ctrl+C) map to current shortcut (e.g., `ctrl+C` -> `copy`)
instead of changing drives
3. cut (`ctrl+X`) followed by paste (`ctrl+V`) translates into a file move as one would expect
4. left and right arrows in the tree view expand and collapse folders like in the Explorer
5. added context menus in both panes
6. improved the means by which icons are displayed for files
7. F12 runs notepad or notepad++ on the selected file
8. moved the ini file location to `%AppData%\Roaming\Microsoft\WinFile`
9. File.Search can include a date which limits the files returned to those after the date provided;
the output is also sorted by the date instead of by the name
10. File.Search includes an option as to whether to include sub-directories
11. `ctrl+K` starts a command shell (ConEmu if installed) in the current directory; `shift+ctrl+K`
starts an elevated command shell (`cmd.exe` only)
12. File.Goto (`ctrl+G`) enables one to type a few words of a path and get a list of directories;
selecting one changes to that directory.  Hardcoded to C:\\ .
13. UI shows reparse points (e.g., Junction Points and Symbolic Links) as such with little icons.
14. added simple forward / back navigation (probably needs to be improved)
15. View command has a new option to sort by date forward (oldest on top); normal date sorting is newest on top

## Changes in original_plus (April 2018)

The source code provided here (in the `src` directory) was copied from the Windows NT 4 source tree in November
2007.  The tag named original_plus contains a very limited set of modifications
from the original sources to enable `WinFile.exe` to run on current Windows.
The most significant changes are:

1. converted to Visual Studio solution; works on VS 2015 and 2017
2. compiles and runs on 64-bit Windows (e.g., GetWindowLong -> GetWindowLongPtr, LONG -> LPARAM)
3. added a few header files which were stored elsewhere in the NT source tree (e.g., wfext.h)
4. deleted some unused files (e.g., winfile.def)
5. converted 64-bit arithmetic from internal libraries to C
6. converted internal shell APIs to public APIs (the primary reason the old version would not run)

The help directory contains both winfile.hlp and winfile.chm.  Winfile.hlp was in the NT4
source tree, but does not work on Windows 10 any more.  Winfile.chm was copied from
a regular installation of Windows 98 and works on Windows 10.  As is, `WinFile.exe`
tries to launch winfile.hlp which fails.

To create your own local branch referring to this release, run `git checkout -b <your branch> original_plus`.

