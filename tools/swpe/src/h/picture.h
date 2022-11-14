#ifndef __PICTURE_H_
#define __PICTURE_H_

/* errors that can occur while dealing with pictures */
enum pic_errors
{
    PIC_NO_ERROR,         /* no error, this picture is ok                    */
    PIC_ERROR_FILE,       /* can't open file                                 */
    PIC_ERROR_READING,    /* error reading file                              */
    PIC_ERROR_NO_MEMORY,  /* out of memory while working on picture          */
    PIC_ERROR_INV_SIZE    /* size is invalid, it is not 64768 bytes          */
};

/* information about one picture */
typedef struct _Sws_256_pic {
    uchar   *filename;          /* filename of the picture                   */
    uchar   *picture;           /* ptr to picture pixels                     */
    uchar   *palette;           /* ptr to picture palette                    */
    uchar   is_loaded:1;        /* flag - is picture loaded?                 */
    uchar   error:3;            /* error code, if there was an error         */
    short   text_color;         /* color to write text with, brightest color */
    struct _Sws_256_pic *next;  /* next picture in list                      */
    struct _Sws_256_pic *prev;  /* previous picture in list                  */
} Sws_256_pic;

/* global information about pictures */
typedef struct _Pictures_info {
    Sws_256_pic *pic_current;   /* ptr to current picture                    */
    signed char gamma;          /* gamma correction value                    */
    schar       old_gamma;      /* previous gamma value                      */
    uchar       is_pal_valid:1; /* is current palette valid?                 */
    uchar       *palette;       /* pointer to gamma corrected palette        */
} Pictures_info;

/* maximum and minimum gamma correction values */
#define MAX_GAMMA  30
#define MIN_GAMMA -15

void LoadPicture(Sws_256_pic *sws_pic);
void DrawPicture(byte **pbits, Sws_256_pic *sws_pic, uint pitch);
bool SavePicture(Sws_256_pic *sws_pic);
void ApplyGammaToPalette();
void CleanUpPictures();

extern const Mode PictureMode;

#endif