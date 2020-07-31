
#define _USE_MATH_DEFINES         
#include <windows.h>            
#include <windowsx.h>           
#include <math.h>              
#include <ocidl.h>           
#include <olectl.h>            
#include <string.h>           
#include "winbgi.h"           
#include "winbgitypes.h"       
#include "dibapi.h"             
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif


WindowData* BGI__GetWindowDataPtr( HWND hWnd )
{
    if ( hWnd == NULL && BGI__CurrentWindow >= 0 && BGI__CurrentWindow < BGI__WindowCount)
        hWnd = BGI__WindowTable[BGI__CurrentWindow];
    if (hWnd == NULL)
    {
	showerrorbox("Drawing operation was attempted when there was no current window.");
	exit(0);
    }
    return (WindowData*)GetWindowLong( hWnd, GWL_USERDATA );
}


HDC BGI__GetWinbgiDC( HWND hWnd )
{
    if ( hWnd == NULL )
        hWnd = BGI__WindowTable[BGI__CurrentWindow];
    WindowData *pWndData = BGI__GetWindowDataPtr( hWnd );

    WaitForSingleObject(pWndData->hDCMutex, 5000);
    return pWndData->hDC[pWndData->ActivePage];
}


void BGI__ReleaseWinbgiDC( HWND hWnd )
{
    if ( hWnd == NULL )
        hWnd = BGI__WindowTable[BGI__CurrentWindow];
    WindowData *pWndData = BGI__GetWindowDataPtr( hWnd );

    ReleaseMutex(pWndData->hDCMutex);
}


void CenterToBox( int x, int y, int xradius, int yradius,
                 int* left, int* top, int* right, int* bottom )
{
    *left   = x - xradius;
    *top    = y - yradius;
    *right  = x + xradius;
    *bottom = y + yradius;
}


void ArcEndPoints( int x, int y, int xradius, int yradius, int stangle,
                  int endangle, int* xstart, int* ystart, int* xend, int* yend )
{
    *xstart = int( xradius * cos( stangle  * M_PI / 180 ) );
    *ystart = int( yradius * sin( stangle  * M_PI / 180 ) );
    *xend   = int( xradius * cos( endangle * M_PI / 180 ) );
    *yend   = int( yradius * sin( endangle * M_PI / 180 ) );

    *xstart += x;
    *ystart  = -*ystart + y;
    *xend   += x;
    *yend    = -*yend + y;
}

void RefreshWindow( RECT* rect )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    POINT p[2];

    if ( rect != NULL )
    {
        p[0].x = rect->left;
        p[0].y = rect->top;
        p[1].x = rect->right;
        p[1].y = rect->bottom;

	hDC = BGI__GetWinbgiDC( );
        LPtoDP( hDC, p, 2 );
        BGI__ReleaseWinbgiDC( );
	
        rect->left = p[0].x;
        rect->top = p[0].y;
        rect->right = p[1].x;
        rect->bottom = p[1].y;
    }

    if (pWndData->refreshing || rect == NULL)
    {    
	if ( pWndData->VisualPage == pWndData->ActivePage )
	    InvalidateRect( pWndData->hWnd, rect, FALSE );
    }
}

bool getrefreshingbgi( )
{
    return BGI__GetWindowDataPtr( )->refreshing;
}


void setrefreshingbgi(bool value)
{
    BGI__GetWindowDataPtr( )->refreshing = value;
}

void refreshallbgi( )
{
    RefreshWindow(NULL);
}

void refreshbgi(int left, int top, int right, int bottom)
{
    RECT rect;
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    POINT p[2];

    p[0].x = min(left, right);
    p[0].y = min(top, bottom);
    p[1].x = max(left, right);
    p[1].y = max(top, bottom);

    hDC = BGI__GetWinbgiDC( );
    LPtoDP( hDC, p, 2 );
    BGI__ReleaseWinbgiDC( );

    rect.left = p[0].x;
    rect.top = p[0].y;
    rect.right = p[1].x;
    rect.bottom = p[1].y;
    
    if ( pWndData->VisualPage == pWndData->ActivePage )
        InvalidateRect( pWndData->hWnd, &rect, FALSE );
}

void arc( int x, int y, int stangle, int endangle, int radius )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int left, top, right, bottom;
    int xstart, ystart, xend, yend;

    CenterToBox( x, y, radius, radius, &left, &top, &right, &bottom );
    ArcEndPoints( x, y, radius, radius, stangle, endangle, &xstart, &ystart, &xend, &yend );

    hDC = BGI__GetWinbgiDC( );
    Arc( hDC, left, top, right, bottom, xstart, ystart, xend, yend );
    BGI__ReleaseWinbgiDC( );
    
    RECT rect = { left, top, right+1, bottom+1 };
    RefreshWindow( &rect );

    pWndData->arcInfo.x = x;
    pWndData->arcInfo.y = y;
    pWndData->arcInfo.xstart = xstart;
    pWndData->arcInfo.ystart = ystart;
    pWndData->arcInfo.xend = xend;
    pWndData->arcInfo.yend = yend;
}

void bar( int left, int top, int right, int bottom )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    HBRUSH hBrush;
    int color;

    hDC = BGI__GetWinbgiDC( );
    hBrush = (HBRUSH)GetCurrentObject( hDC, OBJ_BRUSH );
    color = converttorgb( pWndData->fillInfo.color );
    SetTextColor( hDC, color );
    RECT r = {left, top, right, bottom};
    FillRect( hDC, &r, hBrush );
    color = converttorgb( pWndData->drawColor );
    SetTextColor( hDC, color );
    BGI__ReleaseWinbgiDC( );
    
    RECT rect = { left, top, right+1, bottom+1 };
    RefreshWindow( &rect );
}


void bar3d( int left, int top, int right, int bottom, int depth, int topflag )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int color;
    int dy;            
    POINT p[4];         

    hDC = BGI__GetWinbgiDC( );
    color = converttorgb( pWndData->fillInfo.color );
    SetTextColor( hDC, color );
    Rectangle( hDC, left, top, right, bottom );
    color = converttorgb( pWndData->drawColor );
    SetTextColor( hDC, color );

    dy = (int)(depth * tan( 30.0 * M_PI / 180.0 ));
    
    p[0].x = right;
    p[0].y = bottom;                
    p[1].x = right + depth;
    p[1].y = bottom - dy;           
    p[2].x = right + depth;
    p[2].y = top - dy;              
    p[3].x = right;
    p[3].y = top;                   

    if ( depth != 0 )
        Polyline( hDC, p, 4 );

    if ( topflag != 0 )
    {
        p[0].x = right + depth;
        p[0].y = top - dy;              
        p[1].x = left + depth;
        p[1].y = top - dy;              
        p[2].x = left;
        p[2].y = top;                   
        Polyline( hDC, p, 3 );
    }
    BGI__ReleaseWinbgiDC( );
    
    RECT rect = { left, top-dy, right+depth+1, bottom+1 };
    RefreshWindow( &rect );
}


void circle( int x, int y, int radius )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int left, top, right, bottom;

    CenterToBox( x, y, radius, radius, &left, &top, &right, &bottom );

    hDC = BGI__GetWinbgiDC( );
    Arc( hDC, left, top, right, bottom, x+radius, y, x+radius, y );
    BGI__ReleaseWinbgiDC( );
    
    RECT rect = { left, top, right+1, bottom+1 };
    RefreshWindow( &rect );
}


void cleardevice( )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int color;               
    RECT rect;              
    HRGN hRGN;               
    int is_rgn;                 
    POINT p;                       
    HBRUSH hBrush;           

    color = converttorgb( pWndData->bgColor );

    hDC = BGI__GetWinbgiDC( );
    p.x = 0;
    p.y = 0;
    DPtoLP( hDC, &p, 1 );
    rect.left = p.x;
    rect.top = p.y;
    rect.right = pWndData->width;
    rect.bottom = pWndData->height;

    hRGN = CreateRectRgn( 0, 0, 5, 5 );
    is_rgn = GetClipRgn( hDC, hRGN );
    if ( is_rgn != 0 )
        SelectClipRgn( hDC, NULL );
    
    hBrush = CreateSolidBrush( color );
    FillRect( hDC, &rect, hBrush );
    DeleteObject( hBrush );
    moveto( p.x, p.y );

    if ( is_rgn != 0 )
        SelectClipRgn( hDC, hRGN );
    DeleteRgn( hRGN );
    BGI__ReleaseWinbgiDC( );
    
    RefreshWindow( NULL );
}


void clearviewport( )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int color;
    RECT rect;
    HBRUSH hBrush;
    
    color = converttorgb( pWndData->bgColor );

    rect.left = 0;
    rect.top = 0;
    rect.right = pWndData->viewportInfo.right - pWndData->viewportInfo.left;
    rect.bottom = pWndData->viewportInfo.bottom - pWndData->viewportInfo.top;

    hDC = BGI__GetWinbgiDC( );
    hBrush = CreateSolidBrush( color );
    FillRect( hDC, &rect, hBrush );
    DeleteObject( hBrush );
    BGI__ReleaseWinbgiDC( );
    moveto( 0, 0 );

    RefreshWindow( NULL );
}


void drawpoly(int n_points, int* points) 
{ 
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    
    hDC = BGI__GetWinbgiDC();
    Polyline(hDC, (POINT*)points, n_points);
    BGI__ReleaseWinbgiDC( );
    
    RefreshWindow( NULL );
}


void ellipse( int x, int y, int stangle, int endangle, int xradius, int yradius )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int left, top, right, bottom;
    int xstart, ystart, xend, yend;


    CenterToBox( x, y, xradius, yradius, &left, &top, &right, &bottom );
    ArcEndPoints( x, y, xradius, yradius, stangle, endangle, &xstart, &ystart, &xend, &yend );

    hDC = BGI__GetWinbgiDC( );
    Arc( hDC, left, top, right, bottom, xstart, ystart, xend, yend );
    BGI__ReleaseWinbgiDC( );

    RECT rect = { left, top, right+1, bottom+1 };
    RefreshWindow( &rect );
}


void fillellipse( int x, int y, int xradius, int yradius )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int left, top, right, bottom;
    int color;

    CenterToBox( x, y, xradius, yradius, &left, &top, &right, &bottom );

    hDC = BGI__GetWinbgiDC( );
    color = converttorgb( pWndData->fillInfo.color );
    SetTextColor( hDC, color );
    Ellipse( hDC, left, top, right, bottom );
    color = converttorgb( pWndData->drawColor );
    SetTextColor( hDC, color );
    BGI__ReleaseWinbgiDC( );

    RECT rect = { left, top, right+1, bottom+1 };
    RefreshWindow( &rect );
}


void fillpoly(int n_points, int* points)
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int color;

    hDC = BGI__GetWinbgiDC();
    color = converttorgb( pWndData->fillInfo.color );
    SetTextColor( hDC, color );

    Polygon(hDC, (POINT*)points, n_points);

    color = converttorgb( pWndData->drawColor );
    SetTextColor( hDC, color );
    BGI__ReleaseWinbgiDC( );

    RefreshWindow( NULL );
}


void floodfill( int x, int y, int border )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int color;

    color = converttorgb( pWndData->fillInfo.color );
    border = converttorgb( border );
    hDC = BGI__GetWinbgiDC( );
    SetTextColor( hDC, color );
    FloodFill( hDC, x, y, border );
    color = converttorgb( pWndData->drawColor );
    SetTextColor( hDC, color );
    BGI__ReleaseWinbgiDC( );

    RefreshWindow( NULL );
}


void line( int x1, int y1, int x2, int y2 )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );

    POINT cp;

    hDC = BGI__GetWinbgiDC( );
    MoveToEx( hDC, x1, y1, &cp );
    LineTo( hDC, x2, y2 );
    MoveToEx( hDC, cp.x, cp.y, NULL );
    BGI__ReleaseWinbgiDC( );

    RECT rect = { min(x1,x2), min(y1,y2), max(x1,x2)+1, max(y1,y2)+1 };
    RefreshWindow( &rect );
}


void linerel( int dx, int dy )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );

    POINT cp;

    hDC = BGI__GetWinbgiDC( );
    GetCurrentPositionEx( hDC, &cp );
    LineTo( hDC, cp.x + dx, cp.y + dy );
    BGI__ReleaseWinbgiDC( );

    RECT rect = { min(cp.x,cp.x+dx), min(cp.y,cp.y+dy), max(cp.x,cp.x+dx)+1, max(cp.y,cp.y+dy)+1 };
    RefreshWindow( &rect );
}


void lineto( int x, int y )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );

    POINT cp;

    hDC = BGI__GetWinbgiDC( );
    GetCurrentPositionEx( hDC, &cp );
    LineTo( hDC, x, y );
    BGI__ReleaseWinbgiDC( );

    RECT rect = { min(cp.x,x), min(cp.y,y), max(cp.x,x)+1, max(cp.y,y)+1 };
    RefreshWindow( &rect );
}


void pieslice( int x, int y, int stangle, int endangle, int radius )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int left, top, right, bottom;
    int xstart, ystart, xend, yend;
    int color;


    CenterToBox( x, y, radius, radius, &left, &top, &right, &bottom );
    ArcEndPoints( x, y, radius, radius, stangle, endangle, &xstart, &ystart, &xend, &yend );

    color = converttorgb( pWndData->fillInfo.color );
    hDC = BGI__GetWinbgiDC( );
    SetTextColor( hDC, color );
    Pie( hDC, left, top, right, bottom, xstart, ystart, xend, yend );
    color = converttorgb( pWndData->drawColor );
    SetTextColor( hDC, color );
    BGI__ReleaseWinbgiDC( );

    RECT rect = { left, top, right+1, bottom+1 };
    RefreshWindow( &rect );
}

void putpixel( int x, int y, int color )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );

    color = converttorgb( color );
    hDC = BGI__GetWinbgiDC( );
    SetPixelV( hDC, x, y, color );
    BGI__ReleaseWinbgiDC( );

    RECT rect = { x, y, x+1, y+1 };
    RefreshWindow( &rect );
}


void rectangle( int left, int top, int right, int bottom )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );

    POINT endpoints[5];             
    endpoints[0].x = left;        
    endpoints[0].y = top;
    endpoints[1].x = right;       
    endpoints[1].y = top;
    endpoints[2].x = right;       
    endpoints[2].y = bottom;
    endpoints[3].x = left;        
    endpoints[3].y = bottom;
    endpoints[4].x = left;           
    endpoints[4].y = top;

    hDC = BGI__GetWinbgiDC( );
    Polyline( hDC, endpoints, 5 );
    BGI__ReleaseWinbgiDC( );

    RECT rect = { left, top, right+1, bottom+1 };
    RefreshWindow( &rect );
}


void sector( int x, int y, int stangle, int endangle, int xradius, int yradius )
{
    HDC hDC;
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int left, top, right, bottom;
    int xstart, ystart, xend, yend;
    int color;


    CenterToBox( x, y, xradius, yradius, &left, &top, &right, &bottom );
    ArcEndPoints( x, y, xradius, yradius, stangle, endangle, &xstart, &ystart, &xend, &yend );

    color = converttorgb( pWndData->fillInfo.color );
    hDC = BGI__GetWinbgiDC( );
    SetTextColor( hDC, color );
    Pie( hDC, left, top, right, bottom, xstart, ystart, xend, yend );
    color = converttorgb( pWndData->drawColor );
    SetTextColor( hDC, color );
    BGI__ReleaseWinbgiDC( );

    RECT rect = { left, top, right+1, bottom+1 };
    RefreshWindow( &rect );
}

unsigned int imagesize(int left, int top, int right, int bottom)
{
    long width, height;           
    WindowData* pWndData;         
    HDC hDC;                    
    HDC hMemoryDC;                 
    HBITMAP hOldBitmap;         
    HBITMAP hBitmap;               
    BITMAP b;                   
    long answer;                
    int tries;

    width = 1 + abs(right - left);
    height = 1 + abs(bottom - top);
    pWndData = BGI__GetWindowDataPtr( );
    hDC = BGI__GetWinbgiDC( );

    hMemoryDC = CreateCompatibleDC(hDC);
    hBitmap = CreateCompatibleBitmap(hDC, width, height);
    hOldBitmap = (HBITMAP) SelectObject(hMemoryDC, hBitmap);

    BitBlt(hMemoryDC, 0, 0, width, height, hDC, left, top, SRCCOPY);
    GetObject(hBitmap, sizeof(BITMAP), &b);
    answer = sizeof(BITMAP) + b.bmHeight*b.bmWidthBytes;
    if (answer > UINT_MAX) answer = 0;

    BGI__ReleaseWinbgiDC( );
    SelectObject(hMemoryDC, hOldBitmap);       
    DeleteObject(hBitmap);                    
    DeleteDC(hMemoryDC);                        

    return (unsigned int) answer;
}


void getimage(int left, int top, int right, int bottom, void *bitmap)
{
    long width, height;           
    WindowData* pWndData;         
    HDC hDC;                    
    HDC hMemoryDC;                 
    HBITMAP hOldBitmap;         
    HBITMAP hBitmap;               
    BITMAP* pUser;                  
    long answer;                

    pWndData = BGI__GetWindowDataPtr( );
    hDC = BGI__GetWinbgiDC( );
    width = 1 + abs(right - left);
    height = 1 + abs(bottom - top);

    hMemoryDC = CreateCompatibleDC(hDC);
    hBitmap = CreateCompatibleBitmap(hDC, width, height);
    hOldBitmap = (HBITMAP) SelectObject(hMemoryDC, hBitmap);

    SelectObject(hMemoryDC, hBitmap);
    BitBlt(hMemoryDC, 0, 0, width, height, hDC, left, top, SRCCOPY);
    
    pUser = (BITMAP*) bitmap;
    GetObject(hBitmap, sizeof(BITMAP), pUser);
    pUser->bmBits = (BYTE*) bitmap + sizeof(BITMAP);
    GetBitmapBits(hBitmap, pUser->bmHeight*pUser->bmWidthBytes, pUser->bmBits);

    BGI__ReleaseWinbgiDC( );
    SelectObject(hMemoryDC, hOldBitmap);       
    DeleteObject(hBitmap);                    
    DeleteDC(hMemoryDC);                        
}

void putimage( int left, int top, void *bitmap, int op )
{
    long width, height;           
    WindowData* pWndData;         
    HDC hDC;                    
    HDC hMemoryDC;                 
    HBITMAP hOldBitmap;         
    HBITMAP hBitmap;               
    BITMAP* pUser;                  

    pUser = (BITMAP*) bitmap;
    width = pUser->bmWidth;
    height = pUser->bmHeight;
    pWndData = BGI__GetWindowDataPtr( );
    hDC = BGI__GetWinbgiDC( );
    
    hMemoryDC = CreateCompatibleDC(hDC);
    hBitmap = CreateCompatibleBitmap(hDC, pUser->bmWidth, pUser->bmHeight);
    hOldBitmap = (HBITMAP) SelectObject(hMemoryDC, hBitmap);

    SetBitmapBits(hBitmap, pUser->bmHeight*pUser->bmWidthBytes, pUser->bmBits);

    switch (op)
    {
    case COPY_PUT:
	BitBlt(hDC, left, top, width, height, hMemoryDC, 0, 0, SRCCOPY);
	break;
    case XOR_PUT:
	BitBlt(hDC, left, top, width, height, hMemoryDC, 0, 0, SRCINVERT);
	break;
    case OR_PUT:
	BitBlt(hDC, left, top, width, height, hMemoryDC, 0, 0, SRCPAINT);
	break;
    case AND_PUT:
	BitBlt(hDC, left, top, width, height, hMemoryDC, 0, 0, SRCAND);
	break;
    case NOT_PUT:
	BitBlt(hDC, left, top, width, height, hMemoryDC, 0, 0, NOTSRCCOPY);
	break;
    }
    RefreshWindow( NULL );

    
    BGI__ReleaseWinbgiDC( );
    SelectObject(hMemoryDC, hOldBitmap);       
    DeleteObject(hBitmap);                    
    DeleteDC(hMemoryDC);                        
}

static LPPICTURE readipicture(const char* filename)
{
    HANDLE hFile;             
    DWORD dwSize;            
    HGLOBAL hGlobal;         
    LPVOID pvData;               
    BOOL bRead;              
    DWORD dwBytesRead;          
    LPSTREAM pStr;           
    HRESULT hOK;              
    LPPICTURE pPicture;      

    hFile = CreateFile(
        filename,               
        GENERIC_READ,       
        FILE_SHARE_READ,      
        NULL,              
        OPEN_EXISTING,        
        0,                    
        NULL                   
        );
    if (hFile == INVALID_HANDLE_VALUE) return NULL;

    dwSize = GetFileSize(hFile, NULL);
    if (dwSize == (DWORD)-1)
    {
        CloseHandle(hFile);
        return NULL;
    }

    hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwSize);
    if (hGlobal == NULL)
    {
        CloseHandle(hFile);
	showerrorbox("Insufficient memory to read image");
        return NULL;
    }

    pvData = GlobalLock(hGlobal);
    if (pvData != NULL)
    {
        dwBytesRead = 0;        
        bRead = ReadFile(hFile, pvData, dwSize, &dwBytesRead, NULL);
    }
    GlobalUnlock(hGlobal);
    CloseHandle(hFile);
    if ((pvData == NULL) || !bRead || (dwBytesRead != dwSize))
    {
        GlobalFree(hGlobal);
        return NULL;
    }

    pStr = NULL;       
    hOK = CreateStreamOnHGlobal(hGlobal, TRUE, &pStr);
    if (pStr == NULL)
    {
        GlobalFree(hGlobal);
        return NULL;
    }
    if (FAILED(hOK))
    {
        GlobalFree(hGlobal);
        pStr->Release( );
        return NULL;
    }
    
    hOK = OleLoadPicture(pStr, dwSize, FALSE, IID_IPicture, (LPVOID *)&pPicture);
    pStr->Release( );
    if (!SUCCEEDED(hOK) || (pPicture == NULL))
    {
        GlobalFree(hGlobal);
        return NULL;
    }

    GlobalFree(hGlobal);
    return pPicture;
}

void readimagefile(
    const char* filename,
    int left, int top, int right, int bottom
    )
{
    WindowData* pWndData;         
    HDC hDC;                    
    LPPICTURE pPicture = NULL;                          
    long full_width, full_height;                      
    long width, height;                                
    OPENFILENAME ofn;          
    TCHAR fn[MAX_PATH+1];        
    if (filename == NULL)
    {
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ZeroMemory(&fn, MAX_PATH+1);
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lpstrFilter = _T("Image files (*.bmp, *.gif, *.jpg, *.ico, *.emf, *.wmf)\0*.BMP;*.GIF;*.JPG;*.ICO;*.EMF;*.WMF\0\0");
	ofn.lpstrFile = fn;
	ofn.nMaxFile = MAX_PATH+1;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST ;
	if (!GetOpenFileName(&ofn)) return;
    }

    if (filename == NULL)
	pPicture = readipicture(fn);
    else
	pPicture = readipicture(filename);
    if (pPicture)
    {
	pWndData = BGI__GetWindowDataPtr( );
	hDC = BGI__GetWinbgiDC( );
	width = 1 + abs(right - left);
	height = 1 + abs(bottom - top);
        pPicture->get_Width(&full_width);
        pPicture->get_Height(&full_height);
	pPicture->Render( hDC, left, top, width, height, 0, full_height, full_width, -full_height, NULL);
	BGI__ReleaseWinbgiDC( );
        pPicture->Release( );
	RefreshWindow( NULL );
    }
}    

void writeimagefile(
    const char* filename,
    int left, int top, int right, int bottom,
    bool active, HWND hwnd
    )
{
    long width, height;           
    WindowData* pWndData;         
    HDC hDC;                    
    HDC hMemoryDC;                 
    HBITMAP hOldBitmap;         
    HBITMAP hBitmap;               
    HDIB hDIB;                  
    OPENFILENAME ofn;          
    TCHAR fn[MAX_PATH+1];        
    if (filename == NULL)
    {
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ZeroMemory(&fn, MAX_PATH+1);
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lpstrFilter = _T("Bitmap files (*.bmp)\0*.BMP\0\0");
	ofn.lpstrFile = fn;
	ofn.nMaxFile = MAX_PATH+1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT;
	if (!GetSaveFileName(&ofn)) return;
	if (strlen(fn) < 4 || (fn[strlen(fn)-4] != '.' && strlen(fn) < MAX_PATH-4))
	    strcat(fn, ".BMP");
    }

    pWndData = BGI__GetWindowDataPtr(hwnd);
    WaitForSingleObject(pWndData->hDCMutex, 5000);
    if (active)
	hDC = pWndData->hDC[pWndData->ActivePage];
    else
	hDC = pWndData->hDC[pWndData->VisualPage];
    if (left < 0) left = 0;
    else if (left >= pWndData->width) left = pWndData->width - 1;
    if (right < 0) right = 0;
    else if (right >= pWndData->width) right = pWndData->width;
    if (bottom < 0) bottom = 0;
    else if (bottom >= pWndData->height) bottom = pWndData->height;
    if (top < 0) top = 0;
    else if (top >= pWndData->height) top = pWndData->height;
    width = 1 + abs(right - left);
    height = 1 + abs(bottom - top);

    hMemoryDC = CreateCompatibleDC(hDC);
    hBitmap = CreateCompatibleBitmap(hDC, width, height);

    hOldBitmap = (HBITMAP) SelectObject(hMemoryDC, hBitmap);

    SelectObject(hMemoryDC, hBitmap);
    BitBlt(hMemoryDC, 0, 0, width, height, hDC, left, top, SRCCOPY);

    hDIB = BitmapToDIB(hBitmap, NULL);
    if (filename == NULL)
	SaveDIB(hDIB, fn);
    else
	SaveDIB(hDIB, filename);
    
    ReleaseMutex(pWndData->hDCMutex);
    DestroyDIB(hDIB);
    SelectObject(hMemoryDC, hOldBitmap);       
    DeleteObject(hBitmap);                    
    DeleteDC(hMemoryDC);                        
}

void printimage(
    const char* title,
    double width_inches, double border_left_inches, double border_top_inches,
    int left, int top, int right, int bottom, bool active, HWND hwnd
    )
{
    static PRINTDLG pd_Printer; 
    WindowData* pWndData;         
    long width, height;           
    HDC hMemoryDC;                 
    HDC hDC;                      
    HBITMAP hBitmap;               
    HBITMAP hOldBitmap;         
    int titlelen;             
    int pixels_per_inch_x, pixels_per_inch_y;
    double factor_x, factor_y;
    DOCINFO di;

    if (pd_Printer.hDevNames == NULL && pd_Printer.hDevMode == NULL)
    {
        memset(&pd_Printer, 0, sizeof(PRINTDLG));
        pd_Printer.lStructSize = sizeof(PRINTDLG);
        pd_Printer.Flags = PD_RETURNDEFAULT;
        if (!PrintDlg(&pd_Printer))
            return;  
        pd_Printer.Flags &= ~PD_RETURNDEFAULT;
    }
    pd_Printer.Flags |= PD_RETURNDC;
    if (!PrintDlg(&pd_Printer))
        return;    

    pWndData = BGI__GetWindowDataPtr(hwnd);
    WaitForSingleObject(pWndData->hDCMutex, 5000);
    if (active)
	hDC = pWndData->hDC[pWndData->ActivePage];
    else
	hDC = pWndData->hDC[pWndData->VisualPage];
    if (left < 0) left = 0;
    else if (left >= pWndData->width) left = pWndData->width - 1;
    if (right < 0) right = 0;
    else if (right >= pWndData->width) right = pWndData->width;
    if (bottom < 0) bottom = 0;
    else if (bottom >= pWndData->height) bottom = pWndData->height;
    if (top < 0) top = 0;
    else if (top >= pWndData->height) top = pWndData->height;
    width = 1 + abs(right - left);
    height = 1 + abs(bottom - top);

    hMemoryDC = CreateCompatibleDC(hDC);
    hBitmap = CreateCompatibleBitmap(hDC, width, height);
    hOldBitmap = (HBITMAP) SelectObject(hMemoryDC, hBitmap);
    
    SelectObject(hMemoryDC, hBitmap);
    BitBlt(hMemoryDC, 0, 0, width, height, hDC, left, top, SRCCOPY);

    pixels_per_inch_x = GetDeviceCaps(pd_Printer.hDC, LOGPIXELSX);
    pixels_per_inch_y = GetDeviceCaps(pd_Printer.hDC, LOGPIXELSY);
    factor_x = pixels_per_inch_x * width_inches / width;
    factor_y = factor_x * pixels_per_inch_y / pixels_per_inch_x;

    memset(&di, 0, sizeof(DOCINFO));
    di.cbSize = sizeof(DOCINFO);
    di.lpszDocName = "Windows BGI";

    if (StartDoc(pd_Printer.hDC, &di) != SP_ERROR)
    {   
        StartPage(pd_Printer.hDC);
	if (title == NULL) title = pWndData->title.c_str( );
	titlelen = strlen(title);
	if (titlelen > 0)
	{
	    TextOut(pd_Printer.hDC, int(pixels_per_inch_x*border_left_inches), int(pixels_per_inch_y*border_top_inches), title, titlelen);
	    border_top_inches += 0.25;
	}
        if (GetDeviceCaps(pd_Printer.hDC, RASTERCAPS) & RC_BITBLT)
        {
            StretchBlt(
                pd_Printer.hDC, int(pixels_per_inch_x*border_left_inches), int(pixels_per_inch_y*border_top_inches),
                int(width*factor_x), 
                int(height*factor_y), 
                hMemoryDC, 0, 0, width, height,
                SRCCOPY
                );
        }
        EndPage(pd_Printer.hDC);

        EndDoc(pd_Printer.hDC);
    }

    ReleaseMutex(pWndData->hDCMutex);
    SelectObject(hMemoryDC, hOldBitmap);       
    DeleteObject(hBitmap);                    
    DeleteDC(hMemoryDC);                        
}
