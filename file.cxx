#define     STRICT  

#include <windows.h>      
#include <windowsx.h>      
#include "winbgi.h"         
#include "winbgitypes.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <io.h>
#include <direct.h>
#include <stdlib.h>
#include "dibutil.h"
#include "dibapi.h"   
#include <sstream>                      

#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')    


class BGIout : virtual std::basic_ostringstream<char>
{
public:

};


HANDLE ReadDIBFile(HANDLE);
BOOL SaveDIBFile(void);
BOOL WriteDIB(LPSTR, HANDLE);

HDIB LoadDIB(const char* lpFileName)
{
    HDIB        hDIB;
    HANDLE      hFile;
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    if ((hFile = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
            NULL)) != INVALID_HANDLE_VALUE)
    {
        hDIB = ReadDIBFile(hFile);
        CloseHandle(hFile);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return hDIB;
    }
    else
    {
        showerrorbox("File not found");
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return NULL;
    }
}


WORD SaveDIB(HDIB hDib, const char* lpFileName)
{
    BITMAPFILEHEADER    bmfHdr;     
    LPBITMAPINFOHEADER  lpBI;     
    HANDLE              fh;         
    DWORD               dwDIBSize;
    DWORD               dwWritten;

    if (!hDib)
        return ERR_INVALIDHANDLE;

    fh = CreateFile(lpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (fh == INVALID_HANDLE_VALUE)
        return ERR_OPEN;

    lpBI = (LPBITMAPINFOHEADER)GlobalLock(hDib);
    if (!lpBI)
    {
        CloseHandle(fh);
        return ERR_LOCK;
    }

    if (lpBI->biSize != sizeof(BITMAPINFOHEADER))
    {
        GlobalUnlock(hDib);
        CloseHandle(fh);
        return ERR_NOT_DIB;
    }
    bmfHdr.bfType = DIB_HEADER_MARKER;

    dwDIBSize = *(LPDWORD)lpBI + PaletteSize((LPSTR)lpBI);  


    if ((lpBI->biCompression == BI_RLE8) || (lpBI->biCompression == BI_RLE4))
        dwDIBSize += lpBI->biSizeImage;
    else
    {
        DWORD dwBmBitsSize;       

        dwBmBitsSize = WIDTHBYTES((lpBI->biWidth)*((DWORD)lpBI->biBitCount)) *
                lpBI->biHeight;

        dwDIBSize += dwBmBitsSize;
        lpBI->biSizeImage = dwBmBitsSize;
    }

    bmfHdr.bfSize = dwDIBSize + sizeof(BITMAPFILEHEADER);
    bmfHdr.bfReserved1 = 0;
    bmfHdr.bfReserved2 = 0;
    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + lpBI->biSize +
            PaletteSize((LPSTR)lpBI);

    WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
    
    WriteFile(fh, (LPSTR)lpBI, dwDIBSize, &dwWritten, NULL);

    GlobalUnlock(hDib);
    CloseHandle(fh);

    if (dwWritten == 0)
        return ERR_OPEN;
    else
        return 0;
}

WORD DestroyDIB(HDIB hDib)
{
    GlobalFree(hDib);
    return 0;
}




HANDLE ReadDIBFile(HANDLE hFile)
{
    BITMAPFILEHEADER    bmfHeader;
    DWORD               dwBitsSize;
    UINT                nNumColors;   
    HANDLE              hDIB;        
    HANDLE              hDIBtmp;      
    LPBITMAPINFOHEADER  lpbi;
    DWORD               offBits;
    DWORD               dwRead;


    dwBitsSize = GetFileSize(hFile, NULL);

    hDIB = GlobalAlloc(GMEM_MOVEABLE, (DWORD)(sizeof(BITMAPINFOHEADER) +
            256 * sizeof(RGBQUAD)));

    if (!hDIB)
        return NULL;

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    if (!lpbi) 
    {
        GlobalFree(hDIB);
        return NULL;
    }

 

    if (!ReadFile(hFile, (LPSTR)&bmfHeader, sizeof (BITMAPFILEHEADER),
            &dwRead, NULL))
        goto ErrExit;

    if (sizeof (BITMAPFILEHEADER) != dwRead)
        goto ErrExit;

    if (bmfHeader.bfType != 0x4d42)   
        goto ErrExit;

    if (!ReadFile(hFile, (LPSTR)lpbi, sizeof(BITMAPINFOHEADER), &dwRead,
            NULL))
        goto ErrExit;

    if (sizeof(BITMAPINFOHEADER) != dwRead)
        goto ErrExit;

    if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
        goto ErrExit;
    if (!(nNumColors = (UINT)lpbi->biClrUsed))
    {
        if (lpbi->biBitCount != 24)
            nNumColors = 1 << lpbi->biBitCount; 
    }


    if (lpbi->biClrUsed == 0)
        lpbi->biClrUsed = nNumColors;

    if (lpbi->biSizeImage == 0)
    {
        lpbi->biSizeImage = ((((lpbi->biWidth * (DWORD)lpbi->biBitCount) +
                31) & ~31) >> 3) * lpbi->biHeight;
    }

    GlobalUnlock(hDIB);
    hDIBtmp = GlobalReAlloc(hDIB, lpbi->biSize + nNumColors *
            sizeof(RGBQUAD) + lpbi->biSizeImage, 0);

    if (!hDIBtmp)
        goto ErrExitNoUnlock;
    else
        hDIB = hDIBtmp;

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    ReadFile (hFile, (LPSTR)(lpbi) + lpbi->biSize,
            nNumColors * sizeof(RGBQUAD), &dwRead, NULL);

    offBits = lpbi->biSize + nNumColors * sizeof(RGBQUAD);

    if (bmfHeader.bfOffBits != 0L)
        SetFilePointer(hFile, bmfHeader.bfOffBits, NULL, FILE_BEGIN);

    if (ReadFile(hFile, (LPSTR)lpbi + offBits, lpbi->biSizeImage, &dwRead,
            NULL))
        goto OKExit;


ErrExit:
    GlobalUnlock(hDIB);    

ErrExitNoUnlock:    
    GlobalFree(hDIB);
    return NULL;

OKExit:
    GlobalUnlock(hDIB);
    return hDIB;
}




static bool MouseKindInRange( int kind )
{
    return ( (kind >= WM_MOUSEFIRST) && (kind <= WM_MOUSELAST) );
}

bool ismouseclick( int kind )
{
    WindowData *pWndData = BGI__GetWindowDataPtr( );
    return ( MouseKindInRange( kind ) && pWndData->clicks[kind - WM_MOUSEFIRST].size( ) );
}

void clearmouseclick( int kind )
{
    WindowData *pWndData = BGI__GetWindowDataPtr( );

    if ( MouseKindInRange( kind ) && pWndData->clicks[kind - WM_MOUSEFIRST].size( ) )
        pWndData->clicks[kind - WM_MOUSEFIRST].pop( );
}

void getmouseclick( int kind, int& x, int& y )
{
    WindowData *pWndData = BGI__GetWindowDataPtr( );
    POINTS where;         

    if ( !MouseKindInRange( kind ) )
        return;

    if ( MouseKindInRange( kind ) && pWndData->clicks[kind - WM_MOUSEFIRST].size( ) )
    {
	where = pWndData->clicks[kind - WM_MOUSEFIRST].front( );
        pWndData->clicks[kind - WM_MOUSEFIRST].pop( );
        x = where.x;
        y = where.y;
    }
    else
    {
        x = y = NO_CLICK;
    }
}

void setmousequeuestatus( int kind, bool status )
{
    if ( MouseKindInRange( kind ) )
	BGI__GetWindowDataPtr( )->mouse_queuing[kind - WM_MOUSEFIRST] = status;
}
int mousex( )
{
    WindowData *pWndData = BGI__GetWindowDataPtr( );
    return pWndData->mouse.x;
}

int mousey( )
{
    WindowData *pWndData = BGI__GetWindowDataPtr( );
    return pWndData->mouse.y;
}


void registermousehandler( int kind, void h( int, int ) )
{
    WindowData *pWndData = BGI__GetWindowDataPtr( );
    if ( MouseKindInRange( kind ) )
        pWndData->mouse_handlers[kind - WM_MOUSEFIRST] = h;
}


palettetype *getdefaultpalette( )
{
    static palettetype default_palette = { 16,
                       { BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHTGRAY,
                         DARKGRAY, LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED,
                         LIGHTMAGENTA, YELLOW, WHITE } };

    return &default_palette;
}


void getpalette( palettetype *palette )
{

}


int getpalettesize( )
{
    return MAXCOLORS + 1;
}


void setallpalette( palettetype *palette )
{

}


void setpalette( int colornum, int color )
{

}


void setrgbpalette( int colornum, int red, int green, int blue )
{

}


