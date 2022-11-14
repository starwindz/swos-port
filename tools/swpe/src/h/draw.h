#ifndef __DRAW_H_
#define __DRAW_H_

void DoDraw(HDC hdc);
void UpdateScreen();
bool InitMessages();
void PrintWarning(char *str, uint time, uint align);
void ClearWarning();
byte *CreateDIB(uchar *pal);
void SetPalette(uchar *pal, byte **pbits, bool force);
void HorLine(char *where, uint pitch, uint x1, uint x2, uint y, uint color);
void VerLine(char *where, uint pitch, uint x, uint y1, uint y2, uint color);
void Zoom(char *pbits, uint factor, uint pitch);

#endif