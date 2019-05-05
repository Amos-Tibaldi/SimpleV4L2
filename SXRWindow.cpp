//-----------------------------------------------------------------------
//
// This file is part of the SimpleV4L2 Project
//
//  by Amos Tibaldi - tibaldi at users.sourceforge.net
//
// http://sourceforge.net/projects/simplev4l2/
//
// http://simplev4l2.sourceforge.net/
//
//
// COPYRIGHT: http://www.gnu.org/licenses/gpl.html
//            COPYRIGHT-gpl-3.0.txt
//
//     The SimpleV4L2 Project
//         Simple V4L2 Grabber and RGB Buffer SSSE3 Optimized Video Player
//
//     Copyright (C) 2013 Amos Tibaldi
//
//     This program is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>
#include "SXRWindow.h"
#include "SSSE3RgbToRgba.h"

using namespace std;
using namespace XRTestNamespace::Windowing;

Display * SXRWindow::display;
unsigned short SXRWindow::numberOfWindows;
Window SXRWindow::wArray[MAX_SGLXWINDOW];
SXRWindow * SXRWindow::pwArray[MAX_SGLXWINDOW];
bool SXRWindow::CleanExit;

bool SXRWindow::ShouldExit()
{
    return CleanExit;
}

static XRenderPictFormat* GetRenderRGBA32Format(Display* dpy) {
    static XRenderPictFormat* pictformat = NULL;
    if (pictformat)
        return pictformat;

    XRenderPictFormat templ;
    templ.depth = 32;
    templ.type = PictTypeDirect;
    templ.direct.red = 0;
    templ.direct.green = 8;
    templ.direct.blue = 16;
    templ.direct.redMask = 0xff;
    templ.direct.greenMask = 0xff;
    templ.direct.blueMask = 0xff;
    templ.direct.alphaMask = 0;

    static const unsigned long kMask =
        PictFormatType | PictFormatDepth |
        PictFormatRed | PictFormatRedMask |
        PictFormatGreen | PictFormatGreenMask |
        PictFormatBlue | PictFormatBlueMask |
        PictFormatAlphaMask;

    pictformat = XRenderFindFormat(dpy, kMask, &templ, 0);

    return pictformat;
}



void SXRWindow::redraw( Display *dpy, Window w )
{
    memcpy(theXImage->data, ImageBuffer, ImageBufferSize);

    XImage image;
    memset(&image, 0, sizeof(image));
    image.width = ImageWidth;
    image.height = ImageHeight;
    image.depth = 32;
    image.bits_per_pixel = 32;
    image.format = ZPixmap;
    image.byte_order = LSBFirst;
    image.bitmap_unit = 8;
    image.bitmap_bit_order = LSBFirst;
    image.bytes_per_line = theXImage->bytes_per_line;
    image.red_mask = 0xff;
    image.green_mask = 0xff00;
    image.blue_mask = 0xff0000;
    image.data = theXImage->data;

    int localpixmappixelwidth = max(windowwidth, ImageWidth);
    int localpixmappixelheight = max(windowheight, ImageHeight);
    unsigned long pixmap = XCreatePixmap(display, win, localpixmappixelwidth, localpixmappixelheight, 32);
    GC gc = XCreateGC(display, pixmap, 0, NULL);
    XPutImage(display, pixmap, gc, &image, 0, 0, 0, 0, ImageWidth, ImageHeight);
    XFreeGC(display, gc);

    unsigned long picture = XRenderCreatePicture(display, pixmap, GetRenderRGBA32Format(display), 0, NULL);

    double xscale = (double)((double)ImageWidth/(double)windowwidth);
    double yscale = (double)((double)ImageHeight/(double)windowheight);
    theXTransform.matrix[0][0] = XDoubleToFixed(xscale);
    theXTransform.matrix[1][1] = XDoubleToFixed(yscale);

    XRenderSetPictureTransform(display, picture, &theXTransform);

    XRenderComposite(display, PictOpSrc, picture, 0, thePicture, 0, 0, 0, 0, 0, 0, windowwidth, windowheight);

    XRenderFreePicture(display, picture);
    XFreePixmap(display, pixmap);
}

void SXRWindow::UpdateWindowOnScreen(unsigned char * rgbbuffer)
{
    SSSE3RgbToRgba(ImageWidth, ImageHeight, rgbbuffer, ImageBuffer);

    //unsigned int bcopied = 0;
    //unsigned char * pdest = ImageBuffer;
    //unsigned char * psrc = rgbbuffer;
    //while(bcopied < ImageBufferSize)
    //{
    //    *pdest = *psrc;
    //    *(pdest+1) = *(psrc+1);
    //    *(pdest+2) = *(psrc+2);
    //    pdest+=4;
    //    psrc+=3;
    //    bcopied +=4;
    //}

    redraw(display, win);
}

void SXRWindow::resize( unsigned int width, unsigned int height )
{
    windowwidth = width;
    windowheight = height;
}

void SXRWindow::SetWindowAspectRatio()
{
    xsh.flags = 0;
    xsh.flags |= PAspect;
    xsh.min_aspect.x = ImageWidth;
    xsh.min_aspect.y = ImageHeight;
    xsh.max_aspect.x = ImageWidth;
    xsh.max_aspect.y = ImageHeight;
    XSetWMNormalHints(display, win, &xsh);
}


void SXRWindow::SetWindowTitle(const char * onename)
{
    char * aname = (char*) malloc(256);
    strcpy(aname, onename);
    XTextProperty titleprop;
    XStringListToTextProperty(&aname, 1, &titleprop);
    XSetTextProperty(display, win, &titleprop, XA_WM_NAME);
    XFree(titleprop.value);
    free(aname);
}

void SXRWindow::ToggleFullScreen()
{
    if(!fullscreen)
    {
        XMoveResizeWindow(display, win, 0, 0,  displaywidth,  displayheight);
        XRaiseWindow( display, win);
        XFlush( display);
        windowwidth =  displaywidth;
        windowheight =  displayheight;
        fullscreen = true;
        return;
    }
    else
    {
        XMoveResizeWindow( display, win, 50, 50, ImageWidth, ImageHeight);
        XRaiseWindow( display, win);
        XFlush( display);
        windowwidth = ImageWidth;
        windowheight = ImageHeight;
        fullscreen = false;
    }
}

SXRWindow * SXRWindow::GetWindowPointer(Window w)
{
    for(int i=0; i<numberOfWindows; i++)
    {
        if(wArray[i]==w) return pwArray[i];
    }
    return 0;
}


void SXRWindow::ShortManagementRoutine()
{
    XEvent xevent;
    KeySym ks;
    SXRWindow * punt;
    while(XPending(display))
    {
        XNextEvent(display, & xevent);
        switch (xevent.type) {
        case ConfigureNotify:
            punt = SXRWindow::GetWindowPointer(xevent.xconfigure.window);
            punt->resize( xevent.xconfigure.width, xevent.xconfigure.height );
            break;
        case Expose:
            punt = SXRWindow::GetWindowPointer(xevent.xexpose.window);
            punt->redraw( display, xevent.xexpose.window );
            break;
        case MotionNotify:
            // xevent.xmotion.x);
            break;
        case ButtonPress:
            // = xevent.xbutton.x;
            break;
        case KeyPress:
            ks = XLookupKeysym((XKeyEvent *) & xevent, 0);
            //  XK_Escape XK_Right XK_Left XK_Up XK_Down XK_q
            switch(ks) {
            case XK_f:
            case XK_F:
                punt = SXRWindow::GetWindowPointer(xevent.xexpose.window);
                punt->ToggleFullScreen();
                break;
            case XK_1:
                break;
            case XK_2:
                break;
            case XK_3:
                break;
            case XK_Escape:
                CleanExit = true;
                break;
            default:
                break;
            } // end switch X Keysym

            break;
        case ClientMessage:
            if (*XGetAtomName (display, xevent.xclient.message_type) == *"WM_PROTOCOLS")
            {
                cout <<"Exiting..."<<endl;
                CleanExit = true;
            }

            break;
        } // end switch xevent
    }
    usleep(1000); // to avoid cpu load
}

void SXRWindow::Init()
{
    display = XOpenDisplay(NULL);
    numberOfWindows = 0;
    CleanExit = false;
}

SXRWindow::SXRWindow(int _w, int _h, const char * thename)
{
    windowwidth = ImageWidth = _w;
    windowheight = ImageHeight = _h;
    fullscreen = false;
    strcpy(name, thename);
    ImageBufferSize = ImageWidth*ImageHeight*4;
    ImageBuffer = (unsigned char *)memalign(16, ImageBufferSize);
    memset(ImageBuffer, 255, ImageBufferSize);

    int dummy;
    bool bHaveRender = XRenderQueryExtension(display, &dummy, &dummy);
    if(!bHaveRender) {
        cout << " no XRender"<<endl;
        exit(-1);
    }

    theXImage = XCreateImage(display,
                             DefaultVisual(display, DefaultScreen(display)),
                             DefaultDepth(display, DefaultScreen(display)),
                             ZPixmap,
                             0,
                             static_cast<char*>(malloc(ImageWidth * ImageHeight * 4)),
                             ImageWidth,
                             ImageHeight,
                             32,
                             ImageWidth * 4);

    scrnum = DefaultScreen( display );
    int nvisinfo = -1;
    XVisualInfo * visinfo = XGetVisualInfo(display, scrnum, NULL, &nvisinfo);

    for(int i=0; i<3; i++)
    {
        for(int j=0; j<3; j++)
        {
            theXTransform.matrix[i][j] = XDoubleToFixed(0.0);
        }
    }
    for(int i=0; i<3; i++)
    {
        theXTransform.matrix[i][i] = XDoubleToFixed(1.0);
    }

    root = RootWindow( display, scrnum );


    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap( display, root, visinfo->visual, AllocNone);
    attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | ButtonPressMask  | ButtonReleaseMask;
    mask = CWBackPixel | CWBorderPixel |
           CWColormap |
           CWEventMask;

    win = XCreateWindow( display, root, 0, 0, windowwidth, windowheight, 0,
                         visinfo->depth,
                         InputOutput,
                         visinfo->visual,
                         mask, &attr );
    SetWindowTitle(thename);
    SetWindowAspectRatio();

    XWindowAttributes getattr;
    XGetWindowAttributes(display, win, &getattr);
    XRenderPictFormat * pictformat = XRenderFindVisualFormat(display, getattr.visual);
    thePicture = XRenderCreatePicture(display, win, pictformat, 0, NULL);


    wmDelete=XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, win, &wmDelete, 1);


    displaywidth = XDisplayWidth( display, DefaultScreen( display));
    displayheight = XDisplayHeight( display, DefaultScreen( display));


    XMapWindow( display, win );
    XFlush(display);

    pwArray[numberOfWindows] = this;
    wArray[numberOfWindows] = win;
    numberOfWindows++;

}

void SXRWindow::RemoveReferencesInStaticArrays()
{
    for(int i=0; i<numberOfWindows; i++)
    {
        if(wArray[i]==win)
        {
            for(int j=i; j<numberOfWindows-1; j++)
            {
                pwArray[j] = pwArray[j+1];
                wArray[j] = wArray[j+1];
            }
            numberOfWindows--;
            return;
        }
    }
}

SXRWindow::~SXRWindow ()
{
    RemoveReferencesInStaticArrays();
    free(ImageBuffer);
    XDestroyImage(theXImage);
    XRenderFreePicture(display, thePicture);
    XDestroyWindow(display, win);
    //XCloseDisplay(  display );
}



