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


#ifndef SXRWINDOW_H
#define SXRWINDOW_H

namespace XRTestNamespace
{
namespace Windowing
{

#define MAX_SGLXWINDOW 20

class SXRWindow
{
private:
    Picture thePicture;
    XImage * theXImage;
    XSizeHints xsh;
    char name[256];
    bool fullscreen;
    unsigned char * ImageBuffer;
    int ImageWidth, ImageHeight, ImageBufferSize;
    Window root;
    Window win;
    int scrnum;
    XSetWindowAttributes attr;
    XTransform theXTransform;
    unsigned long mask;
    int windowwidth, windowheight, displaywidth, displayheight;
    void SetWindowTitle(const char * onename);
    static SXRWindow * GetWindowPointer(Window w);
    void SetWindowAspectRatio();
    void RemoveReferencesInStaticArrays();
    Atom wmDelete;
    static Display * display;
    static unsigned short numberOfWindows;
    static Window wArray[MAX_SGLXWINDOW];
    static SXRWindow * pwArray[MAX_SGLXWINDOW];
    static bool CleanExit;
public:
    static bool ShouldExit();
    void ToggleFullScreen();
    void UpdateWindowOnScreen(unsigned char * rgbbuffer);
    void redraw( Display *dpy, Window w );
    void resize( unsigned int width, unsigned int height );
    SXRWindow(int w, int h, const char * name);
    ~SXRWindow();
    static void Init();
    static void ShortManagementRoutine();
};

}

}

#endif
