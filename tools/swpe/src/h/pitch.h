#pragma once

#define MAX_PITCH       6
#define MAX_PITCH_TYPES 7

/* width and height of complete virtual pitch */
#define PITCH_W 672
#define PITCH_H 848

enum {
    kPatternsPerWidth = 21,
    kPatternsPerHeight = 14,
    kVirtualPitchPatternsX = 42,
    kVirtualPitchPatternsY = 55,
};

/* types of pitch in SWOS - palette is set accordingly */
enum g_pitchTypes {
    FROZEN,
    MUDDY,
    WET,
    SOFT,
    NORMAL,
    DRY,
    HARD
};

/* global informations about pitch */
typedef struct Pitch_info {
    short  x;           // x position in pitch
    short  y;           // y       -||-
    short  curx;        // position of editing cursor relative to the current screen, (0, 0) = top-left
    short  cury;
    uchar  curColor;
    schar  curPitch;
    schar  pitchType;  // SWOS pitch type (palette)
    uchar  editMode:1;  // is edit mode active?
    uchar  numbers:1;   // write pattern numbers?
    char   *pal;        // current pitch palette
    ushort curtile;     // starting tile of cached drawn pitch
} PitchInfo;

void DrawPitch(char *where, uint x, uint y, uint pitch, uint pitchType, bool grid, bool numbers);
void SetPitchPalette(uint pattern_no);
void markPitchForRedraw();
void recalculatePatternUsageCount();
bool PitchLeftButtonClicked(bool pressed, int mouseX, int mouseY);
bool PitchLeftButtonDoubleClicked(int mouseX, int mouseY);
void PitchRightButtonClicked(bool pressed, int mouseX, int mouseY);
bool PitchMiddleButtonClicked(bool pressed);
bool PitchMouseMoved(int mouseX, int mouseY, uint buttons);
void PitchMouseScrolled(int delta);
HCURSOR GetPitchCursor();
int getHighlightedPattern(int pitchNo);
void setHighlightedPattern(int pitchNo, int patternNo);
void resetPitchData(bool clearUndo);
bool undoRedoPitchOperation(int pitchNo, bool undo);

#define SWOS_PATTERNS_LIMIT 296 /* maximum number of unique patterns per     */
                                /* pitch that SWOS can handle                */

extern const Mode PitchMode;
