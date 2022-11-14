#ifndef __SWSPR_H_
#define __SWSPR_H_

/* total number of sprites */
#define NUM_SPRITES 1334

/* size of sprite.dat (assertation) */
#define SPRITE_DAT_SIZE 5340

/* number of *.dat file */
#define NUM_DAT_FILES 7

/* sprite structure - from dat files */
#pragma pack(push, _Sprite, 1) /* must pack it for file access */
typedef struct _Sprite {
    char  *spr_data;     /* ptr to actual graphics                           */
    short size;          /* UNUSED - here will be sprite pixels size         */
    short dat_file;      /* UNUSED - here will be containing dat file number */
    uchar changed;       /* UNUSED - here will be changed flag (inserted)    */
    char  unk1;          /* unknown                                          */
    short width;         /* width                                            */
    short nlines;        /* height                                           */
    short wquads;        /* (number of bytes / 8) in one line                */
    short center_x;      /* used when drawing sprite (we ignore it)          */
    short center_y;      /*          -||-                                    */
    uchar unk4;          /* unknown                                          */
    uchar nlines_div4;   /* nlines / 4                                       */
    short ordinal;       /* ordinal number in sprite.dat                     */
} Sprite;
#pragma pack(pop, _Sprite)

/* dat files - contain sprites */
typedef struct _Dat_file {
    char   fname[12];    /* filename                                         */
    HANDLE handle;       /* handle                                           */
    uint   entries;      /* number of sprites                                */
    uint   start_sprite; /* starting sprite in sprite.dat                    */
    uint   size;         /* size of the file                                 */
    uint   old_size;     /* size before changing                             */
    char   *buffer;      /* file contents                                    */
    uchar  changed:1;    /* changed flag                                     */
} Dat_file;

/* sprite index file */
typedef struct _Spr_file {
    char   *fname;
    HANDLE handle;
    uint   size;
    char   *buffer;
} Spr_file;

typedef struct _Sprites_info {
    uchar    bk_col;      /* background color for sprites mode               */
    short    sprite_no;   /* number of sprite currently showing              */
    Spr_file spritef;     /* sprite index file                               */
    Sprite   *sprites;    /* pointer to array of sprite structs              */
    uchar    show_info:1; /* show info about sprite flag                     */
} Sprites_info;

typedef struct _Team_game {
    word pr_shirt_type;
    word pr_shirt_col;
    word pr_stripes_col;
    word pr_shorts_col;
    word pr_socks_col;
    word sec_shirt_type;
    word sec_shirt_col;
    word sec_stripes_col;
    word sec_shorts_col;
    word sec_socks_col;
    word marked_player;
    char team_name[17];
    byte unk_1;
    byte unk_2;
    byte unk_3;
} Team_game;

typedef struct _Player_game {
    byte unk1;
    byte player_index;
    byte goals_scored;
    byte shirt_number;
    byte position;
    byte face;
    byte is_injured;
    byte unk2;
    byte unk3;
    byte unk4;
    byte num_yellow_cards;
    byte unk5;
    char short_name[15];
    byte passing;
    byte shooting;
    byte heading;
    byte tackling;
    byte ball_control;
    byte speed;
    byte finishing;
    byte goalie_direction;
    byte injuries_bitfield;
    byte has_played;
    byte face2;
    char full_name[23];
} Player_game;

void GetSize(Sprite *spr, uint *w, uint *h);
bool SaveSprite(uint sprite_no, uint bkclr, Sprite *spr, uint quiet);
void SaveAllSprites();
bool SaveChangesToSprites();
bool InsertSprite(uint sprite_no, uint quiet);
void InsertAllSprites();
void DrawSprite(int x, int y, uchar *where, Sprite *s, uint pitch, int col);
void DrawSpriteInGame(int x, int y, int sprite_no, uchar *where, uint pitch);
void RevertSpritesToSaved();
void SwapTeam2And3();

extern const Mode SpriteMode;

#endif