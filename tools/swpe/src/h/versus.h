#ifndef __VERSUS_H
#define __VERSUS_H

extern const Mode VersusMode;

typedef struct _PlayerSkill {
    int passing;
    int shooting;
    int heading;
    int tackling;
    int ballControl;
    int speed;
    int finishing;
} PlayerSkill;

typedef struct _AnimationTable {
    int frameDelay;
    int *frameTables[32];
} AnimationTable;

typedef struct _Player {
    PlayerSkill *playerSkill;
    int state;
    int frameOffset;
    AnimationTable *animationTable;
    int *frameIndicesTable;
    int frameIndex;
    int frameDelay;
    int cycleFramesTimer;
    int delayedFramesTimer;
    int x;
    int y;
    int z;
    int direction;
    int speed;
    int deltaX;
    int deltaY;
    int deltaZ;
    int destX;
    int destY;
    int angle;
    int isMoving;
    int wasMoving;
    int pictureIndex;
} Player;

void FinishVersusMode();

#endif