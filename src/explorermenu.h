#include <ShlObj.h>

void ShowExplorerContextMenu(HWND hwnd, LPCWSTR pFileName, UINT xPos, UINT yPos);

extern IContextMenu2 *pExplorerCm2;
extern IContextMenu3 *pExplorerCm3;
