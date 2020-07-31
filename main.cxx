#include "graphics.h"
#include <iostream>
#include <math.h>
#include <stdlib.h>                       
#include <ctype.h>                        
using namespace std;

const double XBORDER = 150.0;                                                        
const double YBORDER = 112.5;                                                        

const double X_RAD = 2 * XBORDER / 10.0;                             
const double Y_RAD = 2 * YBORDER / 12;                      

const double LEFT_X = -1.5 * X_RAD;                           
const double LEFT_Y = 0;                                                      
const double RIGHT_X = 1.5 * X_RAD;                           
const double RIGHT_Y = 0;                                                     

const double I_RADIUS = X_RAD / 2.8;                                         
const double P_RADIUS = X_RAD / 10;                                                 

void draw_iris(int x, int y);

int ytranslate
        (double y,                                                     
    double yborder);                                    

int xtranslate
        (double x,                                                     
    double xborder);                                    


void draw_eyes();
void change_left_color(int x, int y);
void change_right_color(int x, int y);
void check_keys();

bool check_vertical
        (int x,                                                      
    int y,                                                           
    int x1,                                                          
    int y1,                                                          
    double& x_intersect,                               
    double& y_intersect,                               
    int small_ellipse_x_radius,           
    int small_ellipse_y_radius);          

void get_radius_eyes(int& tx_rad, int& ty_rad);

void update_graphics
        (int left_x_intersect,                 
    int left_y_intersect,                      
    int right_x_intersect,                     
    int right_y_intersect,                     
    int ti_rad,                                      
    int tp_rad,                                      
    int& t);                                              

void get_radius_iris_pupil
        (int& ti_rad,                                        
    int& tp_rad);                                            

void find_coordinates
        (int x,                                                      
    int y,                                                           
    int x1,                                                          
    int y1,                                                          
    double& x_intersect,                               
    double& y_intersect,                               
    int small_ellipse_x_radius,           
    int small_ellipse_y_radius);          


int left_color = GREEN;
int right_color = GREEN;


int APIENTRY WinMain
( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{

        cout << "Moving Eyes, Copyright (C) 1998 Grant Macklem.\n" << endl;
   cout << "Press 'Q' to quit" << endl;

   initwindow(601, 451);
    registermousehandler(WM_MOUSEMOVE, draw_iris);
    registermousehandler(WM_LBUTTONDOWN, change_left_color);
    registermousehandler(WM_RBUTTONDOWN, change_right_color);

   draw_iris(2,2);

        for(;;)           
   {
                check_keys();
                delay(10);
   }

}

void draw_eyes()
{
   int x1 = xtranslate(LEFT_X, XBORDER);
   int y1 = ytranslate(LEFT_Y, YBORDER);
   int x2 = xtranslate(RIGHT_X, XBORDER);
   int y2 = ytranslate(RIGHT_Y, YBORDER);
   int tx_rad = xtranslate(X_RAD, XBORDER) - xtranslate(0, XBORDER);
   int ty_rad = ytranslate(Y_RAD, YBORDER) - ytranslate(0, YBORDER);

        setcolor(WHITE);
        fillellipse (x1, y1, tx_rad, ty_rad);
   fillellipse (x2, y2, tx_rad, ty_rad);
}

void change_left_color(int x, int y)
{
        left_color ++;                      
        left_color %= 15;                                 
        draw_iris(mousex(),mousey());
}

void change_right_color(int x, int y)
{
        right_color ++;                             
        right_color %= 15;                        
        draw_iris(mousex(),mousey());
}

void draw_iris(int x, int y)
{
   int tx_rad, ty_rad, ti_rad, tp_rad;
   static int t = 0;              
   double small_ellipse_x_radius;
   double small_ellipse_y_radius;

   int x1 = xtranslate(LEFT_X, XBORDER);
   int y1 = ytranslate(LEFT_Y, YBORDER);
   int x2 = xtranslate(RIGHT_X, XBORDER);
   int y2 = ytranslate(RIGHT_Y, YBORDER);
   double left_x_intersect, left_y_intersect, right_x_intersect, right_y_intersect;

   get_radius_eyes(tx_rad, ty_rad);
   get_radius_iris_pupil(ti_rad, tp_rad);

        small_ellipse_x_radius = tx_rad - ti_rad;
        small_ellipse_y_radius = ty_rad - ti_rad;

        find_coordinates(x, y, x1, y1, left_x_intersect, left_y_intersect,
        small_ellipse_x_radius, small_ellipse_y_radius);
        find_coordinates(x, y, x2, y2, right_x_intersect, right_y_intersect,
        small_ellipse_x_radius, small_ellipse_y_radius);

   update_graphics(left_x_intersect, left_y_intersect, right_x_intersect,
                                         right_y_intersect, ti_rad, tp_rad, t);

}


bool check_vertical
        (int x,                                                      
    int y,                                                           
    int x1,                                                          
    int y1,                                                          
    double& x_intersect,                               
    double& y_intersect,                               
    int small_ellipse_y_radius)           
{
        if (x == x1)
   {
      x_intersect = x;

                if (abs(y1 - y) >= small_ellipse_y_radius)
      {      
        if ((y1 - y) > 0)
                y_intersect = y1 - small_ellipse_y_radius;
         else
                y_intersect = y1 + small_ellipse_y_radius;
      }
      else           
        y_intersect = y;

      return true;
   }
   return false;
}

void find_coordinates
        (int x,                                                      
    int y,                                                           
    int x1,                                                          
    int y1,                                                          
    double& x_intersect,                               
    double& y_intersect,                               
    int small_ellipse_x_radius,           
    int small_ellipse_y_radius)           
{
        bool vertical;
   double numerator, denominator, slope;

        vertical = check_vertical(x, y, x1, y1, x_intersect, y_intersect,
        small_ellipse_y_radius);
   if (!vertical)
        {                    
                slope = ((double)(y1 - y)) / ((double)(x - x1));

                numerator = pow(small_ellipse_x_radius, 2.0) * pow(small_ellipse_y_radius, 2.0);
           denominator = pow(small_ellipse_y_radius, 2.0) + pow(small_ellipse_x_radius, 2.0) * pow(slope, 2.0);

                x_intersect = sqrt(numerator / denominator);
           y_intersect = slope * x_intersect;

           if (x < x1)
                x_intersect = -(abs(x_intersect));
           else
                   x_intersect = abs(x_intersect);

           if (y < y1)
                y_intersect = -(abs(y_intersect));
           else
                   y_intersect = abs(y_intersect);

                x_intersect += x1;
                y_intersect += y1;

           if ( (pow(x - x1,2.0) / pow(small_ellipse_x_radius, 2.0) + pow(y1 - y, 2.0) / pow(small_ellipse_y_radius, 2.0)) < 1)
           {          
                x_intersect = x;
              y_intersect = y;
           }
   }        
}


void get_radius_eyes(int& tx_rad, int& ty_rad)
{
   tx_rad = xtranslate(X_RAD, XBORDER) - xtranslate(0, XBORDER);
   ty_rad = ytranslate(0, YBORDER) - ytranslate(Y_RAD, YBORDER);
}

void get_radius_iris_pupil(int& ti_rad, int& tp_rad)
{
   ti_rad = xtranslate(I_RADIUS, XBORDER) - xtranslate(0, XBORDER);
   tp_rad = xtranslate(P_RADIUS, XBORDER) - xtranslate(0, XBORDER);
}


void update_graphics(int left_x_intersect, int left_y_intersect,
        int right_x_intersect, int right_y_intersect, int ti_rad, int tp_rad, int& t)
{
        int active_page = t % 2;

        setactivepage(active_page);
        clearviewport();
        setfillstyle( SOLID_FILL, WHITE );
   draw_eyes();
   setcolor (left_color);
   setfillstyle( SOLID_FILL, left_color );
   fillellipse(left_x_intersect, left_y_intersect, ti_rad, ti_rad);

   setcolor (BLACK);
   setfillstyle( SOLID_FILL, BLACK );
   fillellipse(left_x_intersect, left_y_intersect, tp_rad, tp_rad);

   setcolor (right_color);
   setfillstyle( SOLID_FILL, right_color );
   fillellipse(right_x_intersect, right_y_intersect, ti_rad, ti_rad);

   setcolor (BLACK);
   setfillstyle( SOLID_FILL, BLACK );
   fillellipse(right_x_intersect, right_y_intersect, tp_rad, tp_rad);

   t++;
   setvisualpage(active_page);
}


void check_keys()
{
        int command;


        if (kbhit()== true)
   {
      command = toupper(getch());
        if (char(command) == 'Q')
        {
        cout << "Exiting..." << endl;
        exit (EXIT_SUCCESS);
      }
   }
}


int xtranslate (double x, double xborder)
{
   double percent;                     
        int maxx = getmaxx();
   int x_value;                    

   percent = x / xborder;              
   x_value = maxx/2 + percent * (maxx/2);

   return (int)x_value;
}


int ytranslate (double y, double yborder)
{
   float percent;                      
        int maxy = getmaxy();
   int y_value;            

   percent = -y / yborder;             
   y_value = maxy/2 + percent * (maxy/2);

   return (int)y_value;
}
