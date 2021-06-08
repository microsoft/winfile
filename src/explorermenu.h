#include <ShlObj.h>

int ShowExplorerContextMenu(HWND hwnd, LPCWSTR pFileName, HMENU hMenuWinFile, UINT xPos, UINT yPos);

extern IContextMenu2 *pExplorerCm2;
extern IContextMenu3 *pExplorerCm3;
