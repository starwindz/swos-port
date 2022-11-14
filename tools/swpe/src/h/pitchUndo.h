#pragma once

#include "pattern.h"

bool undoPitchEdit(int pitchNo, PitchPatterns *patterns, char *msgBuf);
bool redoPitchEdit(int pitchNo, PitchPatterns *patterns, char *msgBuf);
void clearUndoBuffers();
void logPatternChangedForUndo(int destPatternIndex, int oldPattern, int newPattern, byte aggregated, byte changedFlags);
void logPatternDeletionForUndo(int pitchNo, int patternNo, PitchPatterns *patterns, byte changedFlags);
void logTileOptimizationForUndo(PitchPatterns *patterns, byte changedFlags);
void logInsertNewPatternForUndo(int pitchNo, PitchPatterns *patterns);
