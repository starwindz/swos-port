#ifndef __DX_H_
#define __DX_H_

void LoadDirectDraw();
bool InitDirectDraw(HWND hWnd);
void FinishDirectDraw(HWND hWnd);
void RestoreSurfaces();
bool LockSurface(uint *pitch, byte **pbits);
void UnlockSurface();
void DXBlit();
void DXSetPalette(uchar *pal);
bool InitDirectInput();
void FinishDirectInput();
bool DIGetKbdState(byte *buf);
void WaitRetrace();

#endif