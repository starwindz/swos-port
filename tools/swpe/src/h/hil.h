#ifndef __HIL_H_
#define __HIL_H_

#pragma pack(push, _Hil, 1)
typedef struct _Hil_header {
    dword sig1;               /* always 8 - maybe first team offset?         */
    dword sig2;               /* max. file size it seems (10 x 19000 + 3626) */
    uchar team1[1704];        /* first team - ingame structure               */
    uchar team2[1704];        /* second team -       -||-                    */
    char  name[40];           /* name of league, contest, etc.               */
    char  unk[60];            /* perhaps previous field is 100 bytes?        */
    char  game_type[100];     /* game type - friendly, cup, etc.             */
    word  num_replays;        /* number of replays in higlight file          */
    word  team1_goals;        /* self-explanatory                            */
    word  team2_goals;        /*       -||-                                  */
    byte  pitchType;          /* muddy, frozen, etc.                         */
    byte  pitch_no;           /* number of pitch file                        */
    word  max_subs;           /* maximum number of substitutes               */
} Hil_header;
#pragma pack(pop, _Hil)

typedef dword Hil_goal[1900 / 4];

#define GOAL_SIZE 19000

extern Mode HighlightsMode;

#endif