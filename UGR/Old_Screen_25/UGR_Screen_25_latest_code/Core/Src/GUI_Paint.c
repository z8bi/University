/******************************************************************************
* | File      	:   GUI_Paint.c
* | Author      :   Waveshare electronics
* | Function    :	Achieve drawing: draw points, lines, boxes, circles and
*                   their size, solid dotted line, solid rectangle hollow
*                   rectangle, solid circle hollow circle.
* | Info        :
*   Achieve display characters: Display a single character, string, number
*   Achieve time display: adaptive size display time minutes and seconds
*----------------
* |	This version:   V3.1
* | Date        :   2020-08-15
* | Info        :
* -----------------------------------------------------------------------------
* V3.1(2020-08-15):
* 1.Fix: 
*       Paint_DrawNum
*         Fixed a BUG where the Paint_DrawNum function failed to display 0
* 2.Add： Paint_DrawFloatNum
*     Can display FloatNum   
*
* -----------------------------------------------------------------------------
* V3.0(2019-04-18):
* 1.Change: 
*    Paint_DrawPoint(..., DOT_STYLE DOT_STYLE)
* => Paint_DrawPoint(..., DOT_STYLE Dot_Style)
*    Paint_DrawLine(..., LINE_STYLE Line_Style, DOT_PIXEL Dot_Pixel)
* => Paint_DrawLine(..., DOT_PIXEL Line_width, LINE_STYLE Line_Style)
*    Paint_DrawRectangle(..., DRAW_FILL Filled, DOT_PIXEL Dot_Pixel)
* => Paint_DrawRectangle(..., DOT_PIXEL Line_width, DRAW_FILL Draw_Fill)
*    Paint_DrawCircle(..., DRAW_FILL Draw_Fill, DOT_PIXEL Dot_Pixel)
* => Paint_DrawCircle(..., DOT_PIXEL Line_width, DRAW_FILL Draw_Filll)
*
* -----------------------------------------------------------------------------
* V2.0(2018-11-15):
* 1.add: Paint_NewImage()
*    Create an image's properties
* 2.add: Paint_SelectImage()
*    Select the picture to be drawn
* 3.add: Paint_SetRotate()
*    Set the direction of the cache    
* 4.add: Paint_RotateImage() 
*    Can flip the picture, Support 0-360 degrees, 
*    but only 90.180.270 rotation is better
* 4.add: Paint_SetMirroring() 
*    Can Mirroring the picture, horizontal, vertical, origin
* 5.add: Paint_DrawString_CN() 
*    Can display Chinese(GB1312)   
*
* ----------------------------------------------------------------------------- 
* V1.0(2018-07-17):
*   Create library
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documnetation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to  whom the Software is
* furished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*
******************************************************************************/

#include "GUI_Paint.h"
#include "fontBIG.c"
#include "image.c"
#include "font24.c"
#include <stdint.h>
#include <stdlib.h>
#include <string.h> //memset()
#include <math.h>
#include <stdbool.h>
#include <stdio.h>


volatile PAINT Paint;
void (*DISPLAY)(UWORD,UWORD,UWORD);
void (*CLEAR)(UWORD);
/******************************************************************************
function:	Create Image
parameter:
    image   :   Pointer to the image cache
    width   :   The width of the picture
    Height  :   The height of the picture
    Color   :   Whether the picture is inverted
******************************************************************************/
void Paint_NewImage(UWORD Width, UWORD Height, UWORD Rotate, UWORD Color)
{
    Paint.WidthMemory = Width;
    Paint.HeightMemory = Height;
    Paint.Color = Color;    
    Paint.WidthByte = Width;
    Paint.HeightByte = Height;    
    printf("WidthByte = %d, HeightByte = %d\r\n", Paint.WidthByte, Paint.HeightByte);
   
    Paint.Rotate = Rotate;
    Paint.Mirror = MIRROR_NONE;
    
    if(Rotate == ROTATE_0 || Rotate == ROTATE_180) {
        Paint.Width = Width;
        Paint.Height = Height;
    } else {
        Paint.Width = Height;
        Paint.Height = Width;
    }
}
/******************************************************************************
function:	Select Clear Funtion
parameter:
      Clear :   Pointer to Clear funtion 
******************************************************************************/
void Paint_SetClearFuntion(void (*Clear)(UWORD))
{
  CLEAR=Clear;
}
/******************************************************************************
function:	Select DisplayF untion
parameter:
      Display :   Pointer to display funtion 
******************************************************************************/
void Paint_SetDisplayFuntion(void (*Display)(UWORD,UWORD,UWORD))
{
  DISPLAY=Display;
}

/******************************************************************************
function:	Select Image Rotate
parameter:
    Rotate   :   0,90,180,270
******************************************************************************/
void Paint_SetRotate(UWORD Rotate)
{
    if(Rotate == ROTATE_0 || Rotate == ROTATE_90 || Rotate == ROTATE_180 || Rotate == ROTATE_270) {
        Debug("Set image Rotate %d\r\n", Rotate);
        Paint.Rotate = Rotate;
    } else {
        Debug("rotate = 0, 90, 180, 270\r\n");
      //  exit(0);
    }
}

/******************************************************************************
function:	Select Image mirror
parameter:
    mirror   :       Not mirror,Horizontal mirror,Vertical mirror,Origin mirror
******************************************************************************/
void Paint_SetMirroring(UBYTE mirror)
{
    if(mirror == MIRROR_NONE || mirror == MIRROR_HORIZONTAL || 
        mirror == MIRROR_VERTICAL || mirror == MIRROR_ORIGIN) {
        Debug("mirror image x:%s, y:%s\r\n",(mirror & 0x01)? "mirror":"none", ((mirror >> 1) & 0x01)? "mirror":"none");
        Paint.Mirror = mirror;
    } else {
        Debug("mirror should be MIRROR_NONE, MIRROR_HORIZONTAL, \
        MIRROR_VERTICAL or MIRROR_ORIGIN\r\n");
//exit(0);
    }    
}

/******************************************************************************
function:	Draw Pixels
parameter:
    Xpoint  :   At point X
    Ypoint  :   At point Y
    Color   :   Painted colors
******************************************************************************/
void Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color)
{
    if(Xpoint > Paint.Width || Ypoint > Paint.Height){
        Debug("Exceeding display boundaries\r\n");
        return;
    }      
    UWORD X, Y;

    switch(Paint.Rotate) {
    case 0:
        X = Xpoint;
        Y = Ypoint;  
        break;
    case 90:
        X = Paint.WidthMemory - Ypoint - 1;
        Y = Xpoint;
        break;
    case 180:
        X = Paint.WidthMemory - Xpoint - 1;
        Y = Paint.HeightMemory - Ypoint - 1;
        break;
    case 270:
        X = Ypoint;
        Y = Paint.HeightMemory - Xpoint - 1;
        break;

    default:
        return;
    }
    
    switch(Paint.Mirror) {
    case MIRROR_NONE:
        break;
    case MIRROR_HORIZONTAL:
        X = Paint.WidthMemory - X - 1;
        break;
    case MIRROR_VERTICAL:
        Y = Paint.HeightMemory - Y - 1;
        break;
    case MIRROR_ORIGIN:
        X = Paint.WidthMemory - X - 1;
        Y = Paint.HeightMemory - Y - 1;
        break;
    default:
        return;
    }

    // printf("x = %d, y = %d\r\n", X, Y);
    if(X > Paint.WidthMemory || Y > Paint.HeightMemory){
        Debug("Exceeding display boundaries\r\n");
        return;
    }
    
   // UDOUBLE Addr = X / 8 + Y * Paint.WidthByte;
		DISPLAY(X,Y, Color);
}

/******************************************************************************
function:	Clear the color of the picture
parameter:
    Color   :   Painted colors
******************************************************************************/
void Paint_Clear(UWORD Color)
{	
	CLEAR(Color);
}

/******************************************************************************
function:	Clear the color of a window
parameter:
    Xstart :   x starting point
    Ystart :   Y starting point
    Xend   :   x end point
    Yend   :   y end point
******************************************************************************/
void Paint_ClearWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD Color)
{
    UWORD X, Y;
    for (Y = Ystart; Y < Yend; Y++) {
        for (X = Xstart; X < Xend; X++) {//8 pixel =  1 byte
            Paint_SetPixel(X, Y, Color);
        }
    }
}

/******************************************************************************
function:	Draw Point(Xpoint, Ypoint) Fill the color
parameter:
    Xpoint		:   The Xpoint coordinate of the point
    Ypoint		:   The Ypoint coordinate of the point
    Color		:   Set color
    Dot_Pixel	:	point size
******************************************************************************/
void Paint_DrawPoint( UWORD Xpoint,       UWORD Ypoint, UWORD Color,
                      DOT_PIXEL Dot_Pixel,DOT_STYLE Dot_FillWay)
{
    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Paint_DrawPoint Input exceeds the normal display range\r\n");
        return;
    }

    int16_t XDir_Num , YDir_Num;
    if (Dot_FillWay == DOT_FILL_AROUND) {
        for (XDir_Num = 0; XDir_Num < 2*Dot_Pixel - 1; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num < 2 * Dot_Pixel - 1; YDir_Num++) {
                if(Xpoint + XDir_Num - Dot_Pixel < 0 || Ypoint + YDir_Num - Dot_Pixel < 0)
                    break;
                // printf("x = %d, y = %d\r\n", Xpoint + XDir_Num - Dot_Pixel, Ypoint + YDir_Num - Dot_Pixel);
                Paint_SetPixel(Xpoint + XDir_Num - Dot_Pixel, Ypoint + YDir_Num - Dot_Pixel, Color);
            }
        }
    } else {
        for (XDir_Num = 0; XDir_Num <  Dot_Pixel; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num <  Dot_Pixel; YDir_Num++) {
                Paint_SetPixel(Xpoint + XDir_Num - 1, Ypoint + YDir_Num - 1, Color);
            }
        }
    }
}

/******************************************************************************
function:	Draw a line of arbitrary slope
parameter:
    Xstart ：Starting Xpoint point coordinates
    Ystart ：Starting Xpoint point coordinates
    Xend   ：End point Xpoint coordinate
    Yend   ：End point Ypoint coordinate
    Color  ：The color of the line segment
******************************************************************************/
void Paint_DrawLine(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, 
                    UWORD Color, DOT_PIXEL Line_width, LINE_STYLE Line_Style)
{
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        Debug("Paint_DrawLine Input exceeds the normal display range\r\n");
        return;
    }

    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;
    int dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
    int dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;

    // Increment direction, 1 is positive, -1 is counter;
    int XAddway = Xstart < Xend ? 1 : -1;
    int YAddway = Ystart < Yend ? 1 : -1;

    //Cumulative error
    int Esp = dx + dy;
    char Dotted_Len = 0;

    for (;;) {
        Dotted_Len++;
        //Painted dotted line, 2 point is really virtual
        if (Line_Style == LINE_STYLE_DOTTED && Dotted_Len % 3 == 0) {
            //Debug("LINE_DOTTED\r\n");
            Paint_DrawPoint(Xpoint, Ypoint, IMAGE_BACKGROUND, Line_width, DOT_STYLE_DFT);
            Dotted_Len = 0;
        } else {
            Paint_DrawPoint(Xpoint, Ypoint, Color, Line_width, DOT_STYLE_DFT);
        }
        if (2 * Esp >= dy) {
            if (Xpoint == Xend)
                break;
            Esp += dy;
            Xpoint += XAddway;
        }
        if (2 * Esp <= dx) {
            if (Ypoint == Yend)
                break;
            Esp += dx;
            Ypoint += YAddway;
        }
    }
}

/******************************************************************************
function:	Draw a rectangle
parameter:
    Xstart ：Rectangular  Starting Xpoint point coordinates
    Ystart ：Rectangular  Starting Xpoint point coordinates
    Xend   ：Rectangular  End point Xpoint coordinate
    Yend   ：Rectangular  End point Ypoint coordinate
    Color  ：The color of the Rectangular segment
    Filled : Whether it is filled--- 1 solid 0：empty
******************************************************************************/
void Paint_DrawRectangle( UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, 
                          UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Filled )
{
    if (Xstart > Paint.Width || Ystart > Paint.Height ||
        Xend > Paint.Width || Yend > Paint.Height) {
        Debug("Input exceeds the normal display range\r\n");
        return;
    }

    if (Filled ) {
        UWORD Ypoint;
        for(Ypoint = Ystart; Ypoint < Yend; Ypoint++) {
            Paint_DrawLine(Xstart, Ypoint, Xend, Ypoint, Color ,Line_width, LINE_STYLE_SOLID);
        }
    } else {
        Paint_DrawLine(Xstart, Ystart, Xend, Ystart, Color ,Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xstart, Ystart, Xstart, Yend, Color ,Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xend, Yend, Xend, Ystart, Color ,Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xend, Yend, Xstart, Yend, Color ,Line_width, LINE_STYLE_SOLID);
    }
}

/******************************************************************************
function:	Use the 8-point method to draw a circle of the
            specified size at the specified position->
parameter:
    X_Center  ：Center X coordinate
    Y_Center  ：Center Y coordinate
    Radius    ：circle Radius
    Color     ：The color of the ：circle segment
    Filled    : Whether it is filled: 1 filling 0：Do not
******************************************************************************/
void Paint_DrawCircle(  UWORD X_Center, UWORD Y_Center, UWORD Radius, 
                        UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill )
{
    if (X_Center > Paint.Width || Y_Center >= Paint.Height) {
        Debug("Paint_DrawCircle Input exceeds the normal display range\r\n");
        return;
    }

    //Draw a circle from(0, R) as a starting point
    int16_t XCurrent, YCurrent;
    XCurrent = 0;
    YCurrent = Radius;

    //Cumulative error,judge the next point of the logo
    int16_t Esp = 3 - (Radius << 1 );

    int16_t sCountY;
    if (Draw_Fill == DRAW_FILL_FULL) {
        while (XCurrent <= YCurrent ) { //Realistic circles
            for (sCountY = XCurrent; sCountY <= YCurrent; sCountY ++ ) {
                Paint_DrawPoint(X_Center + XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//1
                Paint_DrawPoint(X_Center - XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//2
                Paint_DrawPoint(X_Center - sCountY, Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//3
                Paint_DrawPoint(X_Center - sCountY, Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//4
                Paint_DrawPoint(X_Center - XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//5
                Paint_DrawPoint(X_Center + XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//6
                Paint_DrawPoint(X_Center + sCountY, Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//7
                Paint_DrawPoint(X_Center + sCountY, Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
            }
            if (Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    } else { //Draw a hollow circle
        while (XCurrent <= YCurrent ) {
            Paint_DrawPoint(X_Center + XCurrent, Y_Center + YCurrent, Color, Line_width, DOT_STYLE_DFT);//1
            Paint_DrawPoint(X_Center - XCurrent, Y_Center + YCurrent, Color, Line_width, DOT_STYLE_DFT);//2
            Paint_DrawPoint(X_Center - YCurrent, Y_Center + XCurrent, Color, Line_width, DOT_STYLE_DFT);//3
            Paint_DrawPoint(X_Center - YCurrent, Y_Center - XCurrent, Color, Line_width, DOT_STYLE_DFT);//4
            Paint_DrawPoint(X_Center - XCurrent, Y_Center - YCurrent, Color, Line_width, DOT_STYLE_DFT);//5
            Paint_DrawPoint(X_Center + XCurrent, Y_Center - YCurrent, Color, Line_width, DOT_STYLE_DFT);//6
            Paint_DrawPoint(X_Center + YCurrent, Y_Center - XCurrent, Color, Line_width, DOT_STYLE_DFT);//7
            Paint_DrawPoint(X_Center + YCurrent, Y_Center + XCurrent, Color, Line_width, DOT_STYLE_DFT);//0

            if (Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    }
}

/******************************************************************************
function:	Show English characters
parameter:
    Xpoint           ：X coordinate
    Ypoint           ：Y coordinate
    Acsii_Char       ：To display the English characters
    Font             ：A structure pointer that displays a character size
    Color_Background : Select the background color of the English character
    Color_Foreground : Select the foreground color of the English character
******************************************************************************/

void Paint_DrawChar(UWORD Xpoint, UWORD Ypoint, const char Acsii_Char,
                    sFONT* Font, UWORD Color_Background, UWORD Color_Foreground, bool gay)
{
    UWORD Page, Column;
    uint32_t Char_Offset = 0;


    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Paint_DrawChar Input exceeds the normal display range\r\n");
        return;
    }
    if(!gay) {
    	Char_Offset = (Acsii_Char - ' ') * Font->Height * (Font->Width / 8 + (Font->Width % 8 ? 1 : 0));
    }
    else {
		switch (Acsii_Char) {
			case 32: Font = &FontBIG_NOT; break; // ASCII 'NOTHING'
			case 48: Font = &FontBIG_0; break; // ASCII '0'
			case 49: Font = &FontBIG_1; break; // ASCII '1'
			case 50: Font = &FontBIG_2; break; // ASCII '2'
			case 51: Font = &FontBIG_3; break; // ASCII '3'
			case 52: Font = &FontBIG_4; break; // ASCII '4'
			case 53: Font = &FontBIG_5; break; // ASCII '5'
			case 54: Font = &FontBIG_6; break; // ASCII '6'
			case 55: Font = &FontBIG_7; break; // ASCII '7'
			case 56: Font = &FontBIG_8; break; // ASCII '8'
			case 57: Font = &FontBIG_9; break; // ASCII '9'
		}
    }
    const unsigned char *ptr = &Font->table[Char_Offset];

    for (Page = 0; Page < Font->Height; Page++) {
        for (Column = 0; Column < Font->Width; Column++) {

            if (FONT_BACKGROUND == Color_Background) {
                if (*ptr & (0x80 >> (Column % 8))) {
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
                }
            } else {
                if (*ptr & (0x80 >> (Column % 8))) {
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Foreground);
                } else {
                    Paint_SetPixel(Xpoint + Column, Ypoint + Page, Color_Background);
                }
            }

            // Advance to the next byte in the row every 8 columns
            if (Column % 8 == 7) {
                ptr++;
            }
        }

        // Align to the next row by advancing `ptr` to skip remaining bits
        // Compute bytes per row explicitly
        UWORD bytes_per_row = (Font->Width + 7) / 8;  // Ensure ceiling division for odd widths
        ptr += bytes_per_row - (Column / 8);          // Align `ptr` to the start of the next row
    }
// Write all
}



/******************************************************************************
function:	Display the string
parameter:
    Xstart           ：X coordinate
    Ystart           ：Y coordinate
    pString          ：The first address of the English string to be displayed
    Font             ：A structure pointer that displays a character size
    Color_Background : Select the background color of the English character
    Color_Foreground : Select the foreground color of the English character
******************************************************************************/

void Paint_DrawString_EN(UWORD Xstart, UWORD Ystart, const char * pString,
                         sFONT* Font, UWORD Color_Background, UWORD Color_Foreground, bool gay)
{
    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;

    if (Xstart > Paint.Width || Ystart > Paint.Height) {
        Debug("Paint_DrawString_EN Input exceeds the normal display range\r\n");
        return;
    }

    while (* pString != '\0') {
        //if X direction filled , reposition to(Xstart,Ypoint),Ypoint is Y direction plus the Height of the character
        if ((Xpoint + Font->Width ) > Paint.Width ) {
            Xpoint = Xstart;
            Ypoint += Font->Height;
        }

        // If the Y direction is full, reposition to(Xstart, Ystart)
        if ((Ypoint  + Font->Height ) > Paint.Height ) {
            Xpoint = Xstart;
            Ypoint = Ystart;
        }
        Paint_DrawChar(Xpoint, Ypoint, * pString, Font, Color_Background, Color_Foreground, gay);

        //The next character of the address
        pString ++;

        //The next word of the abscissa increases the font of the broadband
        Xpoint += Font->Width;
        if(gay) {
        	Xpoint += 4;
        }
    }
}


/******************************************************************************
function:	Display the string
parameter:
    Xstart           ：X coordinate
    Ystart           ：Y coordinate
    pString          ：The first address of the Chinese string and English
                        string to be displayed
    Font             ：A structure pointer that displays a character size
    Color_Background : Select the background color of the English character
    Color_Foreground : Select the foreground color of the English character
******************************************************************************/
void Paint_DrawString_CN(UWORD Xstart, UWORD Ystart, const char * pString, cFONT* font, UWORD Color_Background, UWORD Color_Foreground)
{
    const char* p_text = pString;
    int x = Xstart, y = Ystart;
    int i, j,Num;

    /* Send the string character by character on EPD */
    while (*p_text != 0) {
        if(*p_text <= 0x7F) {  //ASCII < 126
            for(Num = 0; Num < font->size; Num++) {
                if(*p_text== font->table[Num].index[0]) {
                    const char* ptr = &font->table[Num].matrix[0];

                    for (j = 0; j < font->Height; j++) {
                        for (i = 0; i < font->Width; i++) {
                            if (FONT_BACKGROUND == Color_Background) { //this process is to speed up the scan
                                if (*ptr & (0x80 >> (i % 8))) {
                                    Paint_SetPixel(x + i, y + j, Color_Foreground);
                                    // Paint_DrawPoint(x + i, y + j, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                }
                            } else {
                                if (*ptr & (0x80 >> (i % 8))) {
                                    Paint_SetPixel(x + i, y + j, Color_Foreground);
                                    // Paint_DrawPoint(x + i, y + j, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                } else {
                                    Paint_SetPixel(x + i, y + j, Color_Background);
                                    // Paint_DrawPoint(x + i, y + j, Color_Background, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                }
                            }
                            if (i % 8 == 7) {
                                ptr++;
                            }
                        }
                        if (font->Width % 8 != 0) {
                            ptr++;
                        }
                    }
                    break;
                }
            }
            /* Point on the next character */
            p_text += 1;
            /* Decrement the column position by 16 */
            x += font->ASCII_Width;
        } else {        //Chinese
            for(Num = 0; Num < font->size; Num++) {
                if((*p_text== font->table[Num].index[0]) && (*(p_text+1) == font->table[Num].index[1])) {
                    const char* ptr = &font->table[Num].matrix[0];

                    for (j = 0; j < font->Height; j++) {
                        for (i = 0; i < font->Width; i++) {
                            if (FONT_BACKGROUND == Color_Background) { //this process is to speed up the scan
                                if (*ptr & (0x80 >> (i % 8))) {
                                    Paint_SetPixel(x + i, y + j, Color_Foreground);
                                    // Paint_DrawPoint(x + i, y + j, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                }
                            } else {
                                if (*ptr & (0x80 >> (i % 8))) {
                                    Paint_SetPixel(x + i, y + j, Color_Foreground);
                                    // Paint_DrawPoint(x + i, y + j, Color_Foreground, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                } else {
                                    Paint_SetPixel(x + i, y + j, Color_Background);
                                    // Paint_DrawPoint(x + i, y + j, Color_Background, DOT_PIXEL_DFT, DOT_STYLE_DFT);
                                }
                            }
                            if (i % 8 == 7) {
                                ptr++;
                            }
                        }
                        if (font->Width % 8 != 0) {
                            ptr++;
                        }
                    }
                    break;
                }
            }
            /* Point on the next character */
            p_text += 2;
            /* Decrement the column position by 16 */
            x += font->Width;
        }
    }
}

/******************************************************************************
function:	Display nummber
parameter:
    Xstart           ：X coordinate
    Ystart           : Y coordinate
    Nummber          : The number displayed
    Font             ：A structure pointer that displays a character size
    Color_Background : Select the background color of the English character
    Color_Foreground : Select the foreground color of the English character
******************************************************************************/
#define  ARRAY_LEN 255

bool WasTripleDigit;

void Paint_DrawNum(UWORD Xpoint, UWORD Ypoint, int32_t Nummber,
                   sFONT* Font, UWORD Color_Background, UWORD Color_Foreground, bool gay, bool drawingSpeed)
{
    int16_t Num_Bit = 0, Str_Bit = 0;
    uint8_t Str_Array[ARRAY_LEN] = {0}, Num_Array[ARRAY_LEN] = {0};
    uint8_t *pStr = Str_Array;

    if (Xpoint > Paint.Width || Ypoint > Paint.Height) {
        Debug("Paint_DisNum Input exceeds the normal display range\r\n");
        return;
    }

    int32_t OriginalNumber = Nummber;

    if(gay == true && ((Nummber / 10) == 0)) {
    	Str_Array[0] = ' ';
        Str_Bit ++;
    }
    //Converts a number to a string
     do{
        Num_Array[Num_Bit] = Nummber % 10 + '0';
        Num_Bit++;
        Nummber /= 10;
    }while (Nummber);

    //The string is inverted
    while (Num_Bit > 0) {
        Str_Array[Str_Bit] = Num_Array[Num_Bit - 1];
        Str_Bit ++;
        Num_Bit --;
    }
    
    if(drawingSpeed & (OriginalNumber / 100 == 0)) {
        	Str_Array[Str_Bit] = ' ';
    }


    //show
    Paint_DrawString_EN(Xpoint, Ypoint, (const char*)pStr, Font, Color_Background, Color_Foreground, gay);
}
/******************************************************************************
function:	Display float number
parameter:
    Xstart           ：X coordinate
    Ystart           : Y coordinate
    Nummber          : The float data that you want to display
	Decimal_Point	 : Show decimal places
    Font             ：A structure pointer that displays a character size
    Color            : Select the background color of the English character
******************************************************************************/

void Paint_DrawFloatNum(UWORD Xpoint, UWORD Ypoint, double Number, UBYTE Decimal_Point,
                        sFONT* Font, UWORD Color_Background, UWORD Color_Foreground, bool gay)
{
    char Str[ARRAY_LEN];  // Buffer for the formatted number
    // Format the number to the specified number of decimal places
    snprintf(Str, sizeof(Str), "%.*f", Decimal_Point, Number); // @suppress("Float formatting support")

    // Display the formatted string on the screen
    Paint_DrawString_EN(Xpoint, Ypoint, (const char*)Str, Font, Color_Foreground, Color_Background, false);
}

/******************************************************************************
function:	Display time
parameter:
    Xstart           ：X coordinate
    Ystart           : Y coordinate
    pTime            : Time-related structures
    Font             ：A structure pointer that displays a character size
    Color            : Select the background color of the English character
******************************************************************************/
void Paint_DrawTime(UWORD Xstart, UWORD Ystart, PAINT_TIME *pTime, sFONT* Font,
                    UWORD Color_Background, UWORD Color_Foreground)
{
    uint8_t value[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

    UWORD Dx = Font->Width;

    //Write data into the cache
    Paint_DrawChar(Xstart                           , Ystart, value[pTime->Hour / 10], Font, Color_Background, Color_Foreground, false);
    Paint_DrawChar(Xstart + Dx                      , Ystart, value[pTime->Hour % 10], Font, Color_Background, Color_Foreground, false);
    Paint_DrawChar(Xstart + Dx  + Dx / 4 + Dx / 2   , Ystart, ':'                    , Font, Color_Background, Color_Foreground, false);
    Paint_DrawChar(Xstart + Dx * 2 + Dx / 2         , Ystart, value[pTime->Min / 10] , Font, Color_Background, Color_Foreground, false);
    Paint_DrawChar(Xstart + Dx * 3 + Dx / 2         , Ystart, value[pTime->Min % 10] , Font, Color_Background, Color_Foreground, false);
    Paint_DrawChar(Xstart + Dx * 4 + Dx / 2 - Dx / 4, Ystart, ':'                    , Font, Color_Background, Color_Foreground, false);
    Paint_DrawChar(Xstart + Dx * 5                  , Ystart, value[pTime->Sec / 10] , Font, Color_Background, Color_Foreground, false);
    Paint_DrawChar(Xstart + Dx * 6                  , Ystart, value[pTime->Sec % 10] , Font, Color_Background, Color_Foreground, false);
}

/******************************************************************************
function:	Display image
parameter:
    image            ：Image start address
    xStart           : X starting coordinates
    yStart           : Y starting coordinates
    xEnd             ：Image width
    yEnd             : Image height
******************************************************************************/
void Paint_DrawImage(const unsigned char *image, UWORD xStart, UWORD yStart, UWORD W_Image, UWORD H_Image) 
{
    int i,j; 
		for(j = 0; j < H_Image; j++){
			for(i = 0; i < W_Image; i++){
				if(xStart+i < Paint.WidthMemory  &&  yStart+j < Paint.HeightMemory)//Exceeded part does not display
					Paint_SetPixel(xStart + i, yStart + j, (*(image + j*W_Image*2 + i*2+1))<<8 | (*(image + j*W_Image*2 + i*2)));
				//Using arrays is a property of sequential storage, accessing the original array by algorithm
				//j*W_Image*2 			   Y offset
				//i*2              	   X offset
			}
		}
      
}
//==================================================================================


void Display_Corner_Words(void) {
    // Define the words to be displayed
    const char *top_left = "SPEED";
    const char *top_right = "CHARGE %";
    const char *bottom_left = "CELL TMP";
    const char *bottom_right = "WATER TMP";

    // Define font and colors
    sFONT* Font = &Font24; // Example font (you can adjust according to your requirements)
    UWORD Color_Background = BLACK;
    UWORD Color_Foreground = WHITE;

    // Define the coordinates for the four corners after 90-degree rotation
    UWORD xTopLeft = 0;
    UWORD yTopLeft = 0;

    UWORD xTopRight = 180; // Right side adjusted to prevent overflow
    UWORD yTopRight = 0;

    UWORD xBottomLeft = 0;
    UWORD yBottomLeft = 240 - Font->Height; // Bottom side

    UWORD xBottomRight = 320- (Font->Width * strlen(bottom_right));
    UWORD yBottomRight = 240 - Font->Height; ;

    // Draw the words at the four corners
    //Paint_DrawString_EN(xTopLeft, yTopLeft, top_left, Font, Color_Background, Color_Foreground);
    Paint_DrawString_EN(xTopLeft, yTopLeft, top_left, Font, Color_Background, Color_Foreground, false);
    Paint_DrawString_EN(xTopRight, yTopRight, top_right, Font, Color_Background, Color_Foreground, false);
    Paint_DrawString_EN(xBottomLeft, yBottomLeft, bottom_left, Font, Color_Background, Color_Foreground, false);
    Paint_DrawString_EN(xBottomRight, yBottomRight, bottom_right, Font, Color_Background, Color_Foreground, false);
}

UWORD get_color_charge(int value) {
    if (value > 75) {
        return GREEN;
    } else if (value > 50) {
        return ORANGE;
    } else {
        return RED;
    }
}

UWORD get_color_temp(int value) {
    if (value > 60) {
        return RED;
    } else if (value > 35) {
        return ORANGE;
    } else {
        return GREEN;
    }
}

void Display_Corner_Numbers(int charge, int cell_tmp, int water_tmp) {

	sFONT* Font_BIG = &FontBIG_0;// Example font (you can adjust according to your requirements)
	UWORD Color_Background = BLACK;
	UWORD Color_Foreground = WHITE;
    // Define the numbers to display below/above the words
    if (charge >= 99) {
        charge = 99;
    }

    if (water_tmp >= 99) {
        water_tmp = 99;
    }

    if (cell_tmp >= 99) {
        cell_tmp = 99;
    }
    int top_right_num = charge;
    int bottom_left_num = cell_tmp;
    int bottom_right_num = water_tmp;

    int xNumTopRightOffset = Font_BIG->Width*2 + 12;
    int xNumBottomRightOffset = Font_BIG->Width*2 + 12;
    int chargeAdjuster = 2;

    if(charge >=100) {
        xNumBottomRightOffset = Font_BIG->Width*3 + 10;
    }

    if(water_tmp >=100) {
        xNumBottomRightOffset = Font_BIG->Width*3 + 10;
    }


    UWORD xNumTopRight = 320 - (Font_BIG->Width*2 + 12); // Adjust right side for a 3-digit number
    UWORD yNumTopRight = (40);  // Place below the top right word

    UWORD xNumBottomLeft = 0;
    UWORD yNumBottomLeft = 130; // Place above the bottom left word

    UWORD xNumBottomRight = 320 - (Font_BIG->Width*2 + 12); // Adjust right side for a 3-digit number
    UWORD yNumBottomRight = 130; // Place above the bottom right word

    // Display the 3-digit numbers below the top words and above the bottom words
    Color_Foreground = get_color_charge(charge);
    Paint_DrawNum(xNumTopRight, yNumTopRight, top_right_num, Font_BIG, Color_Background, Color_Foreground, true, false);
    Color_Foreground = get_color_temp(cell_tmp);
    Paint_DrawNum(xNumBottomLeft, yNumBottomLeft, bottom_left_num, Font_BIG, Color_Background, Color_Foreground, true, false);
    Color_Foreground = get_color_temp(water_tmp);
    Paint_DrawNum(xNumBottomRight, yNumBottomRight, bottom_right_num, Font_BIG, Color_Background, Color_Foreground, true, false);
}

void Display_Corner_Speed(int speed) {

	sFONT* Font_BIG = &FontBIG_0;// Example font (you can adjust according to your requirements)
	UWORD Color_Background = BLACK;
	UWORD Color_Foreground = WHITE;
    // Define the numbers to display below/above the words
    int top_left_num = speed;

    UWORD xNumTopLeft = 0;
    UWORD yNumTopLeft = (40);  // Place below the top left word

    // Display the 3-digit numbers below the top words and above the bottom words
    Paint_DrawNum(xNumTopLeft, yNumTopLeft, top_left_num, Font_BIG, Color_Background, Color_Foreground, true, true);
}

void LCD_2in4_test()
{
	printf("LCD_2IN4_test Demo\r\n");

    printf("LCD_2IN4_ Init and Clear...\r\n");
	LCD_2IN4_Init();
	LCD_2IN4_Clear(WHITE);

    printf("Paint_NewImage\r\n");
	Paint_NewImage(LCD_2IN4_WIDTH,LCD_2IN4_HEIGHT, ROTATE_0, WHITE);

    printf("Set Clear and Display Funtion\r\n");
	Paint_SetClearFuntion(LCD_2IN4_Clear);
	Paint_SetDisplayFuntion(LCD_2IN4_DrawPaint);

     printf("Paint_Clear\r\n");
	Paint_Clear(WHITE);
    DEV_Delay_ms(100);

    printf("Painting...\r\n");
	Paint_SetRotate(ROTATE_0);
	Paint_DrawString_EN (5, 10, "DEMO:",        &Font24,    YELLOW,  RED, false);
	Paint_DrawString_EN (5, 34, "Hello World",  &Font24,    BLUE,    CYAN, false);
	Paint_DrawFloatNum  (5, 150 ,987.6,1,  &Font20,    WHITE,   LIGHTBLUE, false);
	Paint_DrawString_EN (5,170, "WaveShare",    &Font24,    WHITE,   BLUE, false);
	Paint_DrawString_CN (5,190, "΢ѩ����",     &Font24CN,  WHITE,   RED);

	Paint_DrawRectangle (125, 240, 225, 300,    RED     ,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
	Paint_DrawLine      (125, 240, 225, 300,    MAGENTA ,DOT_PIXEL_2X2,LINE_STYLE_SOLID);
	Paint_DrawLine      (225, 240, 125, 300,    MAGENTA ,DOT_PIXEL_2X2,LINE_STYLE_SOLID);

	Paint_DrawCircle(150,100,  25,        BLUE    ,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
	Paint_DrawCircle(180,100,  25,        BLACK   ,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
	Paint_DrawCircle(210,100,  25,        RED     ,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
	Paint_DrawCircle(165,125,  25,        YELLOW  ,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);
	Paint_DrawCircle(195,125,  25,        GREEN   ,DOT_PIXEL_2X2,DRAW_FILL_EMPTY);

	Paint_DrawImage(gImage_1,5,70,60,60);

	LCD_2IN4_Clear(BLACK);

	Paint.Rotate = ROTATE_90;
	Paint_DrawImage(ugracing_logo, 110, 70, 100, 100);

	Display_Corner_Words();
	DEV_Delay_ms(30000);

	printf("quit...\r\n");
	//DEV_Module_Exit();
}

void UGR_FUNCTIONS(void){
	Paint_NewImage(LCD_2IN4_WIDTH,LCD_2IN4_HEIGHT, ROTATE_0, WHITE);

	Paint_SetClearFuntion(LCD_2IN4_Clear);
	Paint_SetDisplayFuntion(LCD_2IN4_DrawPaint);

	LCD_2IN4_Clear(BLACK);

	Paint.Rotate = ROTATE_90;
	Paint_DrawImage(ugracing_logo, 110, 70, 100, 100);

	Display_Corner_Words();
	DEV_Delay_ms(30000);

}

void UGR_LOGO(void){
	Paint_DrawImage(ugracing_logo, 110, 70, 100, 100);
}


