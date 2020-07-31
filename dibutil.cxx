#define     STRICT          

#include <windows.h>
#include <assert.h>
#include "dibapi.h"
#include "dibutil.h"
#include <stdio.h>


HDIB CreateDIB(DWORD dwWidth, DWORD dwHeight, WORD wBitCount)
{
    BITMAPINFOHEADER    bi;               
    LPBITMAPINFOHEADER  lpbi;              
    DWORD               dwLen;              
    HDIB                hDIB;
    DWORD               dwBytesPerLine;      


    if (wBitCount <= 1)
        wBitCount = 1;
    else if (wBitCount <= 4)
        wBitCount = 4;
    else if (wBitCount <= 8)
        wBitCount = 8;
    else if (wBitCount <= 24)
        wBitCount = 24;
    else
        wBitCount = 4;           

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = dwWidth;              
    bi.biHeight = dwHeight;            
    bi.biPlanes = 1;                 
    bi.biBitCount = wBitCount;      
    bi.biCompression = BI_RGB;    
    bi.biSizeImage = 0;               
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    dwBytesPerLine = WIDTHBYTES(wBitCount * dwWidth);
    dwLen = bi.biSize + PaletteSize((LPSTR)&bi) + (dwBytesPerLine * dwHeight);

    hDIB = GlobalAlloc(GHND, dwLen);

    if (!hDIB)
        return NULL;

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    *lpbi = bi;

    GlobalUnlock(hDIB);

    return hDIB;
}


LPSTR FindDIBBits(LPSTR lpDIB)
{
   return (lpDIB + *(LPDWORD)lpDIB + PaletteSize(lpDIB));
}



DWORD DIBWidth(LPSTR lpDIB)
{
    LPBITMAPINFOHEADER   lpbmi;        
    LPBITMAPCOREHEADER   lpbmc;       

    lpbmi = (LPBITMAPINFOHEADER)lpDIB;
    lpbmc = (LPBITMAPCOREHEADER)lpDIB;

    if (lpbmi->biSize == sizeof(BITMAPINFOHEADER))
        return lpbmi->biWidth;
    else           
        return (DWORD)lpbmc->bcWidth;
}


DWORD DIBHeight(LPSTR lpDIB)
{
   LPBITMAPINFOHEADER   lpbmi;        
   LPBITMAPCOREHEADER   lpbmc;       

   lpbmi = (LPBITMAPINFOHEADER)lpDIB;
   lpbmc = (LPBITMAPCOREHEADER)lpDIB;

    if (lpbmi->biSize == sizeof(BITMAPINFOHEADER))
        return lpbmi->biHeight;
    else           
        return (DWORD)lpbmc->bcHeight;
}


WORD PaletteSize(LPSTR lpDIB)
{
    if (IS_WIN30_DIB (lpDIB))
        return (DIBNumColors(lpDIB) * sizeof(RGBQUAD));
    else
        return (DIBNumColors(lpDIB) * sizeof(RGBTRIPLE));
}


WORD DIBNumColors(LPSTR lpDIB)
{
    WORD wBitCount;     


    if (IS_WIN30_DIB(lpDIB))
    {
        DWORD dwClrUsed;

        dwClrUsed = ((LPBITMAPINFOHEADER)lpDIB)->biClrUsed;
        if (dwClrUsed)

        return (WORD)dwClrUsed;
    }

    if (IS_WIN30_DIB(lpDIB))
        wBitCount = ((LPBITMAPINFOHEADER)lpDIB)->biBitCount;
    else
        wBitCount = ((LPBITMAPCOREHEADER)lpDIB)->bcBitCount;

    switch (wBitCount)
    {
        case 1:
            return 2;

        case 4:
            return 16;

        case 8:
            return 256;

        default:
            return 0;
    }
}


HPALETTE CreateDIBPalette(HDIB hDIB)
{
    LPLOGPALETTE        lpPal;               
    HANDLE              hLogPal;             
    HPALETTE            hPal = NULL;        
    int                 i, wNumColors;          
    LPSTR               lpbi;              
    LPBITMAPINFO        lpbmi;               
    LPBITMAPCOREINFO    lpbmc;               
    BOOL                bWinStyleDIB;     

    if (!hDIB)
        return NULL;

    lpbi = (LPSTR) GlobalLock(hDIB);

    lpbmi = (LPBITMAPINFO)lpbi;

    lpbmc = (LPBITMAPCOREINFO)lpbi;

    wNumColors = DIBNumColors(lpbi);

    bWinStyleDIB = IS_WIN30_DIB(lpbi);
    if (wNumColors)
    {
        hLogPal = GlobalAlloc(GHND, sizeof(LOGPALETTE) +
                sizeof(PALETTEENTRY) * wNumColors);

        if (!hLogPal)
        {
            GlobalUnlock(hDIB);
            return NULL;
        }

        lpPal = (LPLOGPALETTE)GlobalLock(hLogPal);

        lpPal->palVersion = PALVERSION;
        lpPal->palNumEntries = wNumColors;

        for (i = 0; i < wNumColors; i++)
        {
            if (bWinStyleDIB)
            {
                lpPal->palPalEntry[i].peRed = lpbmi->bmiColors[i].rgbRed;
                lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
                lpPal->palPalEntry[i].peBlue = lpbmi->bmiColors[i].rgbBlue;
                lpPal->palPalEntry[i].peFlags = 0;
            }
            else
            {
                lpPal->palPalEntry[i].peRed = lpbmc->bmciColors[i].rgbtRed;
                lpPal->palPalEntry[i].peGreen = lpbmc->bmciColors[i].rgbtGreen;
                lpPal->palPalEntry[i].peBlue = lpbmc->bmciColors[i].rgbtBlue;
                lpPal->palPalEntry[i].peFlags = 0;
            }
        }

        hPal = CreatePalette(lpPal);

        if (!hPal)
        {
            GlobalUnlock(hLogPal);
            GlobalFree(hLogPal);
            return NULL;
        }
    }

    GlobalUnlock(hLogPal);
    GlobalFree(hLogPal);
    GlobalUnlock(hDIB);

    return hPal;
}


HBITMAP DIBToBitmap(HDIB hDIB, HPALETTE hPal)
{
    LPSTR       lpDIBHdr, lpDIBBits;          
    HBITMAP     hBitmap;                
    HDC         hDC;                       
    HPALETTE    hOldPal = NULL;        

    if (!hDIB)
        return NULL;

    lpDIBHdr = (LPSTR) GlobalLock(hDIB);

    lpDIBBits = FindDIBBits(lpDIBHdr);

    hDC = GetDC(NULL);
    if (!hDC)
    {
        GlobalUnlock(hDIB);
        return NULL;
    }

    if (hPal)
        hOldPal = SelectPalette(hDC, hPal, FALSE);

    RealizePalette(hDC);

    hBitmap = CreateDIBitmap(hDC, (LPBITMAPINFOHEADER)lpDIBHdr, CBM_INIT,
            lpDIBBits, (LPBITMAPINFO)lpDIBHdr, DIB_RGB_COLORS);

    if (hOldPal)
        SelectPalette(hDC, hOldPal, FALSE);

    ReleaseDC(NULL, hDC);
    GlobalUnlock(hDIB);

    return hBitmap;
}


HDIB BitmapToDIB(HBITMAP hBitmap, HPALETTE hPal)
{
    BITMAP              bm;           
    BITMAPINFOHEADER    bi;           
    LPBITMAPINFOHEADER  lpbi;          
    DWORD               dwLen;          
    HANDLE              hDIB, h;         
    HDC                 hDC;           
    WORD                biBits;        

    if (!hBitmap)
        return NULL;

    if (!GetObject(hBitmap, sizeof(bm), (LPSTR)&bm))
        return NULL;

    if (hPal == NULL)
        hPal = (HPALETTE) GetStockObject(DEFAULT_PALETTE);

    biBits = bm.bmPlanes * bm.bmBitsPixel;

    if (biBits <= 1)
        biBits = 1;
    else if (biBits <= 4)
        biBits = 4;
    else if (biBits <= 8)
        biBits = 8;
    else        
        biBits = 24;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bm.bmWidth;
    bi.biHeight = bm.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = biBits;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    dwLen = bi.biSize + PaletteSize((LPSTR)&bi);

    hDC = GetDC(NULL);

    hPal = SelectPalette(hDC, hPal, FALSE);
    RealizePalette(hDC);

    hDIB = GlobalAlloc(GHND, dwLen);

    if (!hDIB)
    {
      SelectPalette(hDC, hPal, TRUE);
      RealizePalette(hDC);
      ReleaseDC(NULL, hDC);
      return NULL;
    }

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    *lpbi = bi;

    GetDIBits(hDC, hBitmap, 0, (UINT)bi.biHeight, NULL, (LPBITMAPINFO)lpbi,
        DIB_RGB_COLORS);

    bi = *lpbi;
    GlobalUnlock(hDIB);

    if (bi.biSizeImage == 0)
        bi.biSizeImage = WIDTHBYTES((DWORD)bm.bmWidth * biBits) * bm.bmHeight;

    dwLen = bi.biSize + PaletteSize((LPSTR)&bi) + bi.biSizeImage;

    if (h = GlobalReAlloc(hDIB, dwLen, 0))
        hDIB = h;
    else
    {
        GlobalFree(hDIB);
        hDIB = NULL;
        SelectPalette(hDC, hPal, TRUE);
        RealizePalette(hDC);
        ReleaseDC(NULL, hDC);
        return NULL;
    }

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

    if (GetDIBits(hDC, hBitmap, 0, (UINT)bi.biHeight, (LPSTR)lpbi +
            (WORD)lpbi->biSize + PaletteSize((LPSTR)lpbi), (LPBITMAPINFO)lpbi,
            DIB_RGB_COLORS) == 0)
    {
        GlobalUnlock(hDIB);
        hDIB = NULL;
        SelectPalette(hDC, hPal, TRUE);
        RealizePalette(hDC);
        ReleaseDC(NULL, hDC);
        return NULL;
    }

    bi = *lpbi;

    GlobalUnlock(hDIB);
    SelectPalette(hDC, hPal, TRUE);
    RealizePalette(hDC);
    ReleaseDC(NULL, hDC);

    return hDIB;
}


int PalEntriesOnDevice(HDC hDC)
{
    int nColors;     

    nColors = (1 << (GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES)));

    assert(nColors);
    return nColors;
}


HPALETTE GetSystemPalette(void)
{
    HDC hDC;                    
    static HPALETTE hPal = NULL;       
    HANDLE hLogPal;              
    LPLOGPALETTE lpLogPal;       
    int nColors;               

    hDC = GetDC(NULL);

    if (!hDC)
        return NULL;

    nColors = PalEntriesOnDevice(hDC);       

    hLogPal = GlobalAlloc(GHND, sizeof(LOGPALETTE) + nColors *
            sizeof(PALETTEENTRY));

    if (!hLogPal)
        return NULL;

    lpLogPal = (LPLOGPALETTE)GlobalLock(hLogPal);

    lpLogPal->palVersion = PALVERSION;
    lpLogPal->palNumEntries = nColors;

    GetSystemPaletteEntries(hDC, 0, nColors,
            (LPPALETTEENTRY)(lpLogPal->palPalEntry));

    hPal = CreatePalette(lpLogPal);

    GlobalUnlock(hLogPal);
    GlobalFree(hLogPal);
    ReleaseDC(NULL, hDC);

    return hPal;
}


HANDLE AllocRoomForDIB(BITMAPINFOHEADER bi, HBITMAP hBitmap)
{
    DWORD               dwLen;
    HANDLE              hDIB;
    HDC                 hDC;
    LPBITMAPINFOHEADER  lpbi;
    HANDLE              hTemp;

    dwLen = bi.biSize + PaletteSize((LPSTR) &bi);
    hDIB  = GlobalAlloc(GHND,dwLen);

    if (!hDIB)
        return NULL;

    lpbi  = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
    *lpbi = bi;

    hDC   = GetDC(NULL);

    GetDIBits(hDC, hBitmap, 0, (UINT) bi.biHeight, NULL, (LPBITMAPINFO)lpbi,
            DIB_RGB_COLORS);
    ReleaseDC(NULL, hDC);

    if (lpbi->biSizeImage == 0)
        lpbi->biSizeImage = WIDTHBYTES((DWORD)lpbi->biWidth *
                lpbi->biBitCount) * lpbi->biHeight;

    dwLen = lpbi->biSize + PaletteSize((LPSTR) &bi) + lpbi->biSizeImage;

    GlobalUnlock(hDIB);

    if (hTemp = GlobalReAlloc(hDIB,dwLen,0))
        return hTemp;
    else
    {
        GlobalFree(hDIB);
        return NULL;
    }
}


HDIB ChangeDIBFormat(HDIB hDIB, WORD wBitCount, DWORD dwCompression)
{
    HDC                hDC;                
    HBITMAP            hBitmap;            
    BITMAP             Bitmap;             
    BITMAPINFOHEADER   bi;                 
    LPBITMAPINFOHEADER lpbi;                
    HDIB               hNewDIB = NULL;      
    HPALETTE           hPal, hOldPal;        
    WORD               DIBBPP, NewBPP;        
    DWORD              DIBComp, NewComp;    

    if (!hDIB)
        return NULL;

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
    DIBBPP = ((LPBITMAPINFOHEADER)lpbi)->biBitCount;
    DIBComp = ((LPBITMAPINFOHEADER)lpbi)->biCompression;
    GlobalUnlock(hDIB);

    if (wBitCount == 0)
    {
        NewBPP = DIBBPP;
        if ((dwCompression == BI_RLE4 && NewBPP == 4) ||
                (dwCompression == BI_RLE8 && NewBPP == 8) ||
                (dwCompression == BI_RGB))
            NewComp = dwCompression;
        else
            return NULL;
    }
    else if (wBitCount == 1 && dwCompression == BI_RGB)
    {
        NewBPP = wBitCount;
        NewComp = BI_RGB;
    }
    else if (wBitCount == 4)
    {
        NewBPP = wBitCount;
        if (dwCompression == BI_RGB || dwCompression == BI_RLE4)
            NewComp = dwCompression;
        else
            return NULL;
    }
    else if (wBitCount == 8)
    {
        NewBPP = wBitCount;
        if (dwCompression == BI_RGB || dwCompression == BI_RLE8)
            NewComp = dwCompression;
        else
            return NULL;
    }
    else if (wBitCount == 24 && dwCompression == BI_RGB)
    {
        NewBPP = wBitCount;
        NewComp = BI_RGB;
    }
    else
        return NULL;

    hPal = CreateDIBPalette(hDIB);
    if (!hPal)
        return NULL;

    hBitmap = DIBToBitmap(hDIB, hPal);
    if (!hBitmap)
    {
        DeleteObject(hPal);
        return NULL;
    }

    GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);

    bi.biSize               = sizeof(BITMAPINFOHEADER);
    bi.biWidth              = Bitmap.bmWidth;
    bi.biHeight             = Bitmap.bmHeight;
    bi.biPlanes             = 1;
    bi.biBitCount           = NewBPP;
    bi.biCompression        = NewComp;
    bi.biSizeImage          = 0;
    bi.biXPelsPerMeter      = 0;
    bi.biYPelsPerMeter      = 0;
    bi.biClrUsed            = 0;
    bi.biClrImportant       = 0;

    hNewDIB = AllocRoomForDIB(bi, hBitmap);
    if (!hNewDIB)
        return NULL;

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hNewDIB);

    hDC  = GetDC(NULL);
    hOldPal = SelectPalette(hDC, hPal, FALSE);
    RealizePalette(hDC);

    if (!GetDIBits(hDC, hBitmap, 0, (UINT) lpbi->biHeight,
            (LPSTR)lpbi + (WORD)lpbi->biSize + PaletteSize((LPSTR)lpbi),
            (LPBITMAPINFO)lpbi, DIB_RGB_COLORS))
    {
        GlobalUnlock(hNewDIB);
        GlobalFree(hNewDIB);
        hNewDIB = NULL;
    }

    SelectPalette(hDC, hOldPal, TRUE);
    RealizePalette(hDC);
    ReleaseDC(NULL, hDC);

    if (hNewDIB)
        GlobalUnlock(hNewDIB);

    DeleteObject(hBitmap);
    DeleteObject(hPal);

    return hNewDIB;
}


HDIB ChangeBitmapFormat(HBITMAP hBitmap, WORD wBitCount, DWORD dwCompression,
        HPALETTE hPal)
{
    HDC                hDC;            
    HDIB               hNewDIB=NULL;     
    BITMAP             Bitmap;          
    BITMAPINFOHEADER   bi;              
    LPBITMAPINFOHEADER lpbi;             
    HPALETTE           hOldPal=NULL;    
    WORD               NewBPP;           
    DWORD              NewComp;         

    if (!hBitmap)
        return NULL;

    if (wBitCount == 0)
    {
        NewComp = dwCompression;
        if (NewComp == BI_RLE4)
            NewBPP = 4;
        else if (NewComp == BI_RLE8)
            NewBPP = 8;
        else     
            return NULL;
    }
    else if (wBitCount == 1 && dwCompression == BI_RGB)
    {
        NewBPP = wBitCount;
        NewComp = BI_RGB;
    }
    else if (wBitCount == 4)
    {
        NewBPP = wBitCount;
        if (dwCompression == BI_RGB || dwCompression == BI_RLE4)
            NewComp = dwCompression;
        else
            return NULL;
    }
    else if (wBitCount == 8)
    {
        NewBPP = wBitCount;
        if (dwCompression == BI_RGB || dwCompression == BI_RLE8)
            NewComp = dwCompression;
        else
            return NULL;
    }
    else if (wBitCount == 24 && dwCompression == BI_RGB)
    {
        NewBPP = wBitCount;
        NewComp = BI_RGB;
    }
    else
        return NULL;

    GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);

    bi.biSize               = sizeof(BITMAPINFOHEADER);
    bi.biWidth              = Bitmap.bmWidth;
    bi.biHeight             = Bitmap.bmHeight;
    bi.biPlanes             = 1;
    bi.biBitCount           = NewBPP;
    bi.biCompression        = NewComp;
    bi.biSizeImage          = 0;
    bi.biXPelsPerMeter      = 0;
    bi.biYPelsPerMeter      = 0;
    bi.biClrUsed            = 0;
    bi.biClrImportant       = 0;

    hNewDIB = AllocRoomForDIB(bi, hBitmap);
    if (!hNewDIB)
        return NULL;

    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hNewDIB);

    if (hPal)
    {
        hDC  = GetDC(NULL);
        hOldPal = SelectPalette(hDC, hPal, FALSE);
        RealizePalette(hDC);
    }

    if (!GetDIBits(hDC, hBitmap, 0, (UINT) lpbi->biHeight, (LPSTR)lpbi +
            (WORD)lpbi->biSize + PaletteSize((LPSTR)lpbi), (LPBITMAPINFO)lpbi,
            DIB_RGB_COLORS))
    {
        GlobalUnlock(hNewDIB);
        GlobalFree(hNewDIB);
        hNewDIB = NULL;
    }

    if (hOldPal)
    {
        SelectPalette(hDC, hOldPal, TRUE);
        RealizePalette(hDC);
        ReleaseDC(NULL, hDC);
    }

    if (hNewDIB)
        GlobalUnlock(hNewDIB);

    return hNewDIB;
}

