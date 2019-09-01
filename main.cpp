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
#include <unistd.h>
#include <malloc.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>
#include <sstream>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libv4l2.h>
#include <libv4lconvert.h>

#include <linux/videodev2.h>

#include "SimpleV4L2.h"
#include "SXRWindow.h"

using namespace SimpleV4L2Namespace;
using namespace XRTestNamespace::Windowing;
using namespace std;

#define XPIX 1920
#define YPIX 1080

int main(int argc, char **argv)
{
    unsigned int frame_counter = 0;
    unsigned char * RgbBuf;
    unsigned char * GrayBuf;

    RgbBuf = (unsigned char*)memalign(16, XPIX*YPIX*3);
    GrayBuf = (unsigned char*)memalign(16, XPIX*YPIX*3);

    SXRWindow::Init();
    SXRWindow w(XPIX, YPIX, "vid");
    SXRWindow wg(XPIX, YPIX, "gray");

    CSimpleV4L2 v(XPIX, YPIX, 0);

    bool bRet = v.Init();

    if(!bRet)
    {
        return -1;
    }

    v.StartStreaming();

    for(; !SXRWindow::ShouldExit(); )
    {
        if(v.NextFrame(RgbBuf))
        {
            frame_counter++;
            cout << ((frame_counter%10==0)?"x":".");
            fflush(stdout);
            w.UpdateWindowOnScreen(RgbBuf);

	    unsigned char * srcoff = RgbBuf;
            unsigned char * dstoff = GrayBuf;
            for(int j=0; j<YPIX; j++)
            {
              for(int i=0; i<XPIX; i++)
              {
		unsigned int mean = *srcoff++;
                mean += *srcoff++;
                mean += *srcoff++;
                mean /=3;
                mean = (mean<0?0:mean);
                mean = (mean>255?255:mean);
                unsigned char ucmean = mean;
                *dstoff++ = ucmean;
                *dstoff++ = ucmean;
                *dstoff++ = ucmean;
              }
            }

           wg.UpdateWindowOnScreen(GrayBuf);
        }

        SXRWindow::ShortManagementRoutine();
    }

    v.StopStreaming();

    v.Reset();

    free(RgbBuf);
    free(GrayBuf);

    return 0;
}


