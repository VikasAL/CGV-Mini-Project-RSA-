
#include <windows.h>           
#include <windowsx.h>           
#include "winbgi.h"           
#include "winbgitypes.h"       



struct LinePattern
{
    int width;
    DWORD pattern[16];
};



LinePattern SOLID  = { 2, {16, 0} };               
LinePattern DOTTED = { 2, {3, 4} };           
LinePattern CENTER = { 4, {3, 4, 7, 4} };       
LinePattern DASHED = {2, {7, 4} };            

COLORREF BGI__Colors[16];

LinePattern CreateUserStyle( );


int converttorgb( int color )
{
    if ( IS_BGI_COLOR( color ) )
        color = BGI__Colors[color];
    else
        color &= 0x0FFFFFF;

    return color;
}

#include <iostream>
void CreateNewPen( )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int color = pWndData->drawColor;;
    LinePattern style;
    LOGBRUSH lb;
    HPEN hPen;

    color = converttorgb( color );

    lb.lbColor = color;
    lb.lbStyle = BS_SOLID;

    if ( pWndData->lineInfo.linestyle == SOLID_LINE )   style = SOLID;
    if ( pWndData->lineInfo.linestyle == DOTTED_LINE )  style = DOTTED;
    if ( pWndData->lineInfo.linestyle == CENTER_LINE )  style = CENTER;
    if ( pWndData->lineInfo.linestyle == DASHED_LINE )  style = DASHED;
    if ( pWndData->lineInfo.linestyle == USERBIT_LINE ) style = CreateUserStyle( );

    WaitForSingleObject(pWndData->hDCMutex, 5000);
    for ( int i = 0; i < MAX_PAGES; i++ )
    {
        hPen = ExtCreatePen( PS_GEOMETRIC | PS_ENDCAP_SQUARE 
                              | PS_JOIN_BEVEL | PS_USERSTYLE,     
                             pWndData->lineInfo.thickness,        
                             &lb,                                 
                             style.width,                          
                             style.pattern );                     
        DeletePen( (HPEN)SelectObject( pWndData->hDC[i], hPen ) );
    }
    ReleaseMutex(pWndData->hDCMutex);
}


LinePattern CreateUserStyle( )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int style = pWndData->lineInfo.upattern;
    int zeroCount = 0;                  
    int i = 0, j, sum = 0;                   
    LinePattern userPattern;      

    style &= 0xFFFF;                 

    if ( style == 0 )
    {
        userPattern.pattern[0] = 0;
        userPattern.pattern[1] = 16;
        userPattern.width = 2;
        return userPattern;
    }

    if ( (style & 1) == 0 )
    {
        for ( j = 0; !( style & 1 ); j++ ) style >>= 1;
        zeroCount = j;
        sum += j;
    }

    while( true )
    {
        for ( j = 0; style & 1; j++ ) style >>= 1;
        userPattern.pattern[i++] = j-1;                         
        sum += j;


        if ( style == 0 )
        {
            if ( sum != 16 )
                userPattern.pattern[i++] = 16 - sum + 1;        
            break;
        }

        for ( j = 0; !( style & 1 ); j++ ) style >>= 1;
        userPattern.pattern[i++] = j + 1;                       
        sum += j;
    }

    if ( zeroCount > 0 )
    {
        if ( (i % 2) == 0 )
            userPattern.pattern[i-1] += zeroCount;
        else
            userPattern.pattern[i++] = zeroCount;
    }
    else               
    {
        if ( (i % 2) != 0 )
            userPattern.pattern[i++] = 0;
    }

    userPattern.width = i;

    return userPattern;
}




void delay( int msec )
{
    Sleep( msec );
}


void getarccoords( arccoordstype *arccoords )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );

    *arccoords = pWndData->arcInfo;
}


int getbkcolor( )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );

    return pWndData->bgColor;
}


int getcolor( )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );

    return pWndData->drawColor;
}


void getfillpattern( char *pattern )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );

    memcpy( pattern, pWndData->uPattern, sizeof( pWndData->uPattern ) );
}


void getfillsettings( fillsettingstype *fillinfo )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    *fillinfo = pWndData->fillInfo;
}


void getlinesettings( linesettingstype *lineinfo )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    *lineinfo = pWndData->lineInfo;
}


int getmaxcolor( )
{
    return WHITE;
}


int getmaxx( )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    return pWndData->width - 1;
}


int getmaxy( )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    return pWndData->height- 1;
}

int getmaxheight( )
{
    int CaptionHeight = GetSystemMetrics( SM_CYCAPTION );       
    int yBorder = GetSystemMetrics( SM_CYFIXEDFRAME );         
    int TotalHeight = GetSystemMetrics( SM_CYSCREEN );         
    
    return TotalHeight - (CaptionHeight + 2*yBorder);          
}

int getmaxwidth( )
{
    int xBorder = GetSystemMetrics( SM_CXFIXEDFRAME );         
    int TotalWidth = GetSystemMetrics( SM_CXSCREEN );          
    
    return TotalWidth - (2*xBorder);          
}

int getwindowheight( )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int CaptionHeight = GetSystemMetrics( SM_CYCAPTION );       
    int yBorder = GetSystemMetrics( SM_CYFIXEDFRAME );         

    return pWndData->height + CaptionHeight + 2*yBorder;       
}

int getwindowwidth( )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    int xBorder = GetSystemMetrics( SM_CXFIXEDFRAME );         

    return pWndData->width + 2*xBorder;                        
}

int COLOR(int r, int g, int b)
{
    COLORREF color = RGB(r,g,b);
    int i;

    for (i = 0; i <= WHITE; i++)
    {
	if ( color == BGI__Colors[i] )
	    return i;
    }

    return ( 0x03000000 | color );
}

int getdisplaycolor( int color )
{
    int save = getpixel( 0, 0 );
    int answer;
    
    putpixel( 0, 0, color );
    answer = getpixel( 0, 0 );
    putpixel( 0, 0, save );
    return answer;
}

int getpixel( int x, int y )
{
    HDC hDC = BGI__GetWinbgiDC( );
    COLORREF color = GetPixel( hDC, x, y );
    BGI__ReleaseWinbgiDC( );
    int i;

    if ( color == CLR_INVALID )
        return CLR_INVALID;

    for ( i = 0; i <= WHITE; i++ )
    {
        if ( color == BGI__Colors[i] )
            return i;
    }

    color |= 0x03000000;

    return color;
}


void getviewsettings( viewporttype *viewport )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    *viewport = pWndData->viewportInfo;
}


int getx( )
{
    HDC hDC = BGI__GetWinbgiDC( );
    POINT cp;

    GetCurrentPositionEx( hDC, &cp );
    BGI__ReleaseWinbgiDC( );
    return cp.x;
}


int gety( )
{
    HDC hDC = BGI__GetWinbgiDC( );
    POINT cp;

    GetCurrentPositionEx( hDC, &cp );
    BGI__ReleaseWinbgiDC( );
    return cp.y;
}


void moverel( int dx, int dy )
{
    HDC hDC = BGI__GetWinbgiDC( );
    POINT cp;

    GetCurrentPositionEx( hDC, &cp );
    MoveToEx( hDC, cp.x + dx, cp.y + dy, NULL );
    BGI__ReleaseWinbgiDC( );
}


void moveto( int x, int y )
{
    HDC hDC = BGI__GetWinbgiDC( );

    MoveToEx( hDC, x, y, NULL );
    BGI__ReleaseWinbgiDC( );
}


void setbkcolor( int color )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );

    pWndData->bgColor = color;

    color = converttorgb( color );

    WaitForSingleObject(pWndData->hDCMutex, 5000);
    for ( int i = 0; i < MAX_PAGES; i++ )
        SetBkColor( pWndData->hDC[i], color );
    ReleaseMutex(pWndData->hDCMutex);
}


void setcolor( int color )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );

    pWndData->drawColor = color;

    color = converttorgb( color );

    WaitForSingleObject(pWndData->hDCMutex, 5000);
    for ( int i = 0; i < MAX_PAGES; i++ )
        SetTextColor( pWndData->hDC[i], color );
    ReleaseMutex(pWndData->hDCMutex);

    CreateNewPen( );
}


void setlinestyle( int linestyle, unsigned upattern, int thickness )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    pWndData->lineInfo.linestyle = linestyle;
    pWndData->lineInfo.upattern = upattern;
    pWndData->lineInfo.thickness = thickness;

    CreateNewPen( );
}


void setfillpattern( char *upattern, int color )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    HBITMAP hBitmap;
    HBRUSH hBrush;
    unsigned short pattern[8];
    int i;

    memcpy( pWndData->uPattern, upattern, sizeof( pWndData->uPattern ) );

    for ( i = 0; i < 8; i++ )
        pattern[i] = (unsigned char)~upattern[i];           

    pWndData->fillInfo.pattern = USER_FILL;
    pWndData->fillInfo.color = color;

    hBitmap = CreateBitmap( 8, 8, 1, 1, pattern );
    WaitForSingleObject(pWndData->hDCMutex, 5000);
    for ( int i = 0; i < MAX_PAGES; i++ )
    {
        hBrush = CreatePatternBrush( hBitmap );
        DeleteBrush( (HBRUSH)SelectBrush( pWndData->hDC[i], hBrush ) );
    }
    ReleaseMutex(pWndData->hDCMutex);
    DeleteBitmap( hBitmap );

}


void setfillstyle( int pattern, int color )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    HDC hDC = BGI__GetWinbgiDC( );
    HBRUSH hBrush;
    unsigned short Slash[8]      = { ~0xE0, ~0xC1, ~0x83, ~0x07, ~0x0E, ~0x1C, ~0x38, ~0x70 };
    unsigned short BkSlash[8]    = { ~0x07, ~0x83, ~0xC1, ~0xE0, ~0x70, ~0x38, ~0x1C, ~0x0E };
    unsigned short Interleave[8] = { ~0xCC, ~0x33, ~0xCC, ~0x33, ~0xCC, ~0x33, ~0xCC, ~0x33 };
    unsigned short WideDot[8]    = { ~0x80, ~0x00, ~0x08, ~0x00, ~0x80, ~0x00, ~0x08, ~0x00 };
    unsigned short CloseDot[8]   = { ~0x88, ~0x00, ~0x22, ~0x00, ~0x88, ~0x00, ~0x22, ~0x00 };
    HBITMAP hBitmap;

    color = converttorgb( color );

    switch ( pattern )
    {
    case EMPTY_FILL:
        hBrush = CreateSolidBrush( converttorgb( pWndData->bgColor ));
        break;
    case SOLID_FILL:
        hBrush = CreateSolidBrush( color );
        break;
    case LINE_FILL:
        hBrush = CreateHatchBrush( HS_HORIZONTAL, color );
        break;
    case LTSLASH_FILL:
        hBrush = CreateHatchBrush( HS_BDIAGONAL, color );
        break;
    case SLASH_FILL:
        hBitmap = CreateBitmap( 8, 8, 1, 1, Slash );
        hBrush = CreatePatternBrush( hBitmap );
        DeleteBitmap( hBitmap );
        break;
    case BKSLASH_FILL:
        hBitmap = CreateBitmap( 8, 8, 1, 1, BkSlash );
        hBrush = CreatePatternBrush( hBitmap );
        DeleteBitmap( hBitmap );
        break;
    case LTBKSLASH_FILL:
        hBrush = CreateHatchBrush( HS_FDIAGONAL, color );
        break;
    case HATCH_FILL:
        hBrush = CreateHatchBrush( HS_CROSS, color );
        break;
    case XHATCH_FILL:
        hBrush = CreateHatchBrush( HS_DIAGCROSS, color );
        break;
    case INTERLEAVE_FILL:
        hBitmap = CreateBitmap( 8, 8, 1, 1, Interleave );
        hBrush = CreatePatternBrush( hBitmap );
        DeleteBitmap( hBitmap );
        break;
    case WIDE_DOT_FILL:
        hBitmap = CreateBitmap( 8, 8, 1, 1, WideDot );
        hBrush = CreatePatternBrush( hBitmap );
        DeleteBitmap( hBitmap );
        break;
    case CLOSE_DOT_FILL:
        hBitmap = CreateBitmap( 8, 8, 1, 1, CloseDot );
        hBrush = CreatePatternBrush( hBitmap );
        DeleteBitmap( hBitmap );
        break;
    case USER_FILL:
        return;
        break;
    default:
        pWndData->error_code = grError;
        return;
    }

    pWndData->fillInfo.pattern = pattern;
    pWndData->fillInfo.color = color;

    DeleteBrush( (HBRUSH)SelectBrush( hDC, hBrush ) );
    BGI__ReleaseWinbgiDC( );
}


void setviewport( int left, int top, int right, int bottom, int clip )
{
    WindowData* pWndData = BGI__GetWindowDataPtr( );
    HRGN hRGN = NULL;
    
    pWndData->viewportInfo.left = left;
    pWndData->viewportInfo.top = top;
    pWndData->viewportInfo.right = right;
    pWndData->viewportInfo.bottom = bottom;
    pWndData->viewportInfo.clip = clip;
    
    if ( clip != 0 )
        hRGN = CreateRectRgn( left, top, right, bottom );

    WaitForSingleObject(pWndData->hDCMutex, 5000);
    for ( int i = 0; i < MAX_PAGES; i++ )
    {
        SelectClipRgn( pWndData->hDC[i], hRGN );

        SetViewportOrgEx( pWndData->hDC[i], left, top, NULL );

        MoveToEx( pWndData->hDC[i], 0, 0, NULL );
    }
    ReleaseMutex(pWndData->hDCMutex);
    DeleteRgn( hRGN );
}


void setwritemode( int mode )
{
    HDC hDC = BGI__GetWinbgiDC( );

    if ( mode == COPY_PUT )
        SetROP2( hDC, R2_COPYPEN );
    if ( mode == XOR_PUT )
        SetROP2( hDC, R2_XORPEN );
    BGI__ReleaseWinbgiDC( );
}


