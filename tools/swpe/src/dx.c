#define INITGUID /* must define this for linker to work properly */

#include <ddraw.h>
#include <dinput.h>
#include "dx.h"
#include "debug.h"

extern unsigned char pal[768];
typedef HRESULT (WINAPI *dd_create_func)(GUID *lpGUID, LPDIRECTDRAW *lplpDD,
                                         IUnknown *pUnkOuter);
static dd_create_func dd_create;

static IDirectDraw        *DD;     /* Direct Draw object                     */
static IDirectDrawSurface *prsurf; /* primary surface                        */
static IDirectDrawSurface *csurf;  /* current memory surface                 */
static IDirectDrawPalette *dpal;   /* DX palette                             */

/* LoadDirectDraw

   Dinamically load ddraw.dll and import DirectDrawCreate function. This way
   program will work on systems without DirectX.
*/
void LoadDirectDraw()
{
    HINSTANCE hddraw = LoadLibrary("ddraw.dll");
    if (hddraw)
        dd_create =
            (dd_create_func)GetProcAddress(hddraw, "DirectDrawCreate");
    else
        WriteToLog(("LoadDirectDraw(): Could not load ddraw.dll"));
}


/* InitDirectDraw

   hWnd - handle of main window

   Switches to DX fullscreen mode. Creates primary surface, and current surface
   in system memory for drawing offscreen. Creates and sets palette. Called as
   many times as user requests it. If fails, user must immediately call
   FinishDirectDraw().
*/
bool InitDirectDraw(HWND hWnd)
{
    DDSURFACEDESC ddsd;
    PALETTEENTRY pe[256] = {0};
    HRESULT result;

    if (!dd_create)
        return FALSE;

    WriteToLog(("InitDirectDraw(): Trying to intialize DirectX..."));
    if (dd_create(NULL, &DD, NULL) != DD_OK) {
        WriteToLog(("InitDirectDraw(): Failed to create DirectDraw object."));
        DD = 0;
        return FALSE;
    }
    WriteToLog(("InitDirectDraw(): DirectDraw object created OK."));
    if (IDirectDraw_SetCooperativeLevel(DD, hWnd, DDSCL_FULLSCREEN |
        DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT) != DD_OK) {
        WriteToLog(("InitDirectDraw(): Failed to set cooperative level."));
        return FALSE;
    }
    WriteToLog(("InitDirectDraw(): Cooperative level set OK."));
    //if ((result = IDirectDraw_SetDisplayMode(DD, WIDTH, HEIGHT, 8)) != DD_OK) {
    if ((result = IDirectDraw_SetDisplayMode(DD, 800, 600, 8)) != DD_OK) {
        WriteToLog(("InitDirectDraw(): Failed to set display mode, result = %#x.", result));
        return FALSE;
    }
    WriteToLog(("InitDirectDraw(): Display mode set OK."));
    memset(&ddsd,0,sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_STANDARDVGAMODE;
    if (IDirectDraw_CreateSurface(DD, &ddsd, &prsurf, NULL) != DD_OK) {
        WriteToLog(("InitDirectDraw(): Failed to create primary surface."));
        prsurf = 0;
        return FALSE;
    }
    WriteToLog(("InitDirectDraw(): Primary surface created OK."));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = WIDTH;
    ddsd.dwHeight = HEIGHT;
    /* working surface will be in system memory */
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    if (IDirectDraw_CreateSurface(DD, &ddsd, &csurf, 0) != DD_OK) {
        WriteToLog(("InitDirectDraw(): Failed to create working surface."));
        csurf = 0;
        return FALSE;
    }
    WriteToLog(("InitDirectDraw(): Working surface created OK."));

    if (IDirectDraw_CreatePalette(DD, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
                                  pe, &dpal, 0) != DD_OK) {
        WriteToLog(("InitDirectDraw(): Palette creation error."));
        dpal = 0;
        return FALSE;
    }

    WriteToLog(("InitDirectDraw(): Palette created OK."));

    if (IDirectDrawSurface_SetPalette(prsurf, dpal) != DD_OK) {
        WriteToLog(("InitDirectDraw(): Could not set palette."));
        return FALSE;
    }
    WriteToLog(("InitDirectDraw(): Palette set OK."));

    WriteToLog(("InitDirectDraw(): DirectX initialized OK."));
    return TRUE;
}


/* FinishDirectDraw

   hWnd - handle of main window

   Switches to window mode from DX fullscreen mode. Flips to GDI surface and
   releases surfaces.
*/
void FinishDirectDraw(HWND hWnd)
{
    PALETTEENTRY pe[256] = {0};

    WriteToLog(("FinishDirectDraw(): Finishing DirectX..."));

    if (DD && dpal) {
        IDirectDraw_WaitForVerticalBlank(DD, DDWAITVB_BLOCKBEGIN, 0);
        IDirectDrawPalette_SetEntries(dpal, 0, 0, 256, pe);
    }
    if (DD) {
        WriteToLog(("FinishDirectDraw(): Fliping to GDI surface and restoring "
                    "display mode."));
        IDirectDraw_FlipToGDISurface(DD);
        IDirectDraw_SetCooperativeLevel(DD, hWnd, DDSCL_NORMAL |
                                        DDSCL_ALLOWREBOOT);
        IDirectDraw_RestoreDisplayMode(DD);
    }
    if (dpal)
        IDirectDrawPalette_Release(dpal);
    dpal = 0;
    if (prsurf)
        IDirectDrawSurface_Release(prsurf);
    prsurf = 0;
    if (csurf)
        IDirectDrawSurface_Release(csurf);
    csurf = 0;
    if (DD)
        IDirectDraw_Release(DD);
    DD = 0;
}


/* RestoreSurfaces

   Restore surfaces, usually after regaining focus.
*/
void RestoreSurfaces()
{
    if (prsurf)
        IDirectDrawSurface_Restore(prsurf);
    if (csurf)
        IDirectDrawSurface_Restore(csurf);
}


/* LockSurface

   pitch -> will get distance between lines if all ok
   pbits -> will get pointer to buffer to draw to if all ok

   Locks our memory surface, and makes it directly accessible to pixel
   plotting. Since it's explicitely created in system memory (not in VRAM), it
   may not be obligatory to lock it, but we're playing safe.
*/
bool LockSurface(uint *pitch, byte **pbits)
{
    DDSURFACEDESC ddsd;
    int s_ok;

    if (!g.fscreen) {
        WriteToLog(("LockSurface(): Lock surface called from window mode!"));
        return FALSE;
    }
    /* actually happened once (during debugging though) */
    if (!csurf)
        return FALSE;

    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    DXWriteToLog(("LockSurface(): Trying to lock syrface..."));
    s_ok = IDirectDrawSurface_Lock(csurf, 0, &ddsd, DDLOCK_WAIT,0) == DD_OK;
    if (s_ok) {
        DXWriteToLog(("LockSurface(): Surface locked OK."));
        *pbits = ddsd.lpSurface;
        *pitch = ddsd.lPitch;
        return TRUE;
    } else {
        DXWriteToLog(("LockSurface(): Surface locking failed."));
        return FALSE;
    }
}


/* UnlockSurface

   Just unlocks current surface. Call only after LockSurface() succeeded.
*/
void UnlockSurface()
{
    DXWriteToLog(("UnlockSurface(): Trying to unlock surface..."));
    DXCheckResult(IDirectDrawSurface_Unlock(csurf, NULL));
}


/* DXBlit

   Makes contents of current surface visible by copying it to primary surface.
*/
void DXBlit()
{
    if (prsurf && csurf)
        IDirectDrawSurface_BltFast(prsurf, 0, 0, csurf, 0,
            DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT);
}


/* DXSetPalette

   pal  - pointer to palette to set

   Sets dpal object to colors in pal. Pal is 768 bytes array of R, G, B values,
   consecutive.
*/
void DXSetPalette(uchar *pal)
{
    uint i;
    PALETTEENTRY pe[256];

    WriteToLog(("DXSetPalette(): Entering DXSetPalette..."));
    for (i = 0; i < 256; i++) {
        pe[i].peRed = pal[3 * i];
        pe[i].peGreen = pal[3 * i + 1];
        pe[i].peBlue = pal[3 * i + 2];
        pe[i].peFlags = 0;
    }
    /* wait for retrace to start... still not 100% tear free, but this is as
       close as I can get to it... */
    IDirectDraw_WaitForVerticalBlank(DD, DDWAITVB_BLOCKBEGIN, 0);
    DXCheckResult(IDirectDrawPalette_SetEntries(dpal, 0, 0, 256, pe));
    WriteToLog(("DXSetPalette(): Exiting DXSetPalette..."));
}


typedef HRESULT (WINAPI *di_create_func)(HINSTANCE hinst, DWORD dwVersion,
                                         LPDIRECTINPUT *lplpDirectInput,
                                         LPUNKNOWN punkOuter);
static di_create_func di_create;
static LPDIRECTINPUT di;
static LPDIRECTINPUTDEVICE dk;

/* InitDirectInput

   Prepare DirectInput for use. Called only once on program start.
*/
bool InitDirectInput()
{
    HRESULT hr;
    HINSTANCE hdinput = LoadLibrary("dinput.dll");
    if (!hdinput) {
        WriteToLog(("InitDirectInput(): Could not load dinput.dll"));
        return FALSE;
    }
    di_create = (di_create_func)GetProcAddress(hdinput, "DirectInputCreateA");
    if (!di_create)
        return FALSE;
    hr = di_create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, &di, NULL);
    if (hr != DI_OK) {
        WriteToLog(("InitDirectInput(): Failed to create "
                    "DirectInput object."));
        return FALSE;
    }
    if (IDirectInput_CreateDevice(di, &GUID_SysKeyboard, &dk, NULL) != DI_OK){
        WriteToLog(("InitDirectInput(): Failed to create device."));
        FinishDirectInput();
        return FALSE;
    }
    if (IDirectInputDevice_SetDataFormat(dk, &c_dfDIKeyboard) != DI_OK) {
        WriteToLog(("InitDirectInput(): Failed to set device data format."));
        FinishDirectInput();
        return FALSE;
    }
    hr = IDirectInputDevice_SetCooperativeLevel(dk, g.hWnd, DISCL_FOREGROUND |
                                                DISCL_NONEXCLUSIVE);
    if (hr != DI_OK) {
        WriteToLog(("InitDirectInput(): Failed to set cooperation level."));
        FinishDirectInput();
        return FALSE;
    }
    /* ignore this result; errors will show up when trying to read the data */
    IDirectInputDevice_Acquire(dk);
    return TRUE;
}


/* FinishDirectInput

   Releases resources associated with Direct Input.
*/
void FinishDirectInput()
{
    if (!di)
        return;
    if (dk) {
        IDirectInputDevice_Unacquire(dk);
        IDirectInputDevice_Release(dk);
        dk = NULL;
    }
    IDirectInput_Release(di);
    di = NULL;
}


/* DIGetKbdState

   buf - buffer for keyboard state, must be big enough to take 256 characters

   Reads keyboard state using DirectInput. Returns FALSE if any error is
   detected with DirectInput.
*/
bool DIGetKbdState(byte *buf)
{
    HRESULT hr;
    for (;;) {
        hr = IDirectInputDevice_GetDeviceState(dk, 256 * sizeof(char), buf);
        if (hr != DIERR_INPUTLOST && hr != DIERR_NOTACQUIRED)
            break;
        if (IDirectInputDevice_Acquire(dk) != DI_OK) {
            return FALSE;
        }
    }
    return hr == DI_OK;
}


/* WaitRetrace

   Waits vertical retrace interval, but only if we're in DX fullscreen mode.
*/
void WaitRetrace()
{
    if (DD)
        IDirectDraw_WaitForVerticalBlank(DD, DDWAITVB_BLOCKBEGIN, 0);
}