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
#include <thread>
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
#include <sys/time.h>

#include <linux/videodev2.h>

#include "SimpleV4L2.h"
#include "SXRWindow.h"

using namespace SimpleV4L2Namespace;
using namespace XRTestNamespace::Windowing;
using namespace std;

#define XPIX 1920
#define YPIX 1080
#define ALSO_GRAY_WINDOW
struct timeval tv;
struct timezone tz;
unsigned int lastsec = -1;
unsigned int last_frame_counter = 0;
unsigned int frame_counter = 0;

unsigned char * RgbBuf;
#ifdef ALSO_GRAY_WINDOW
unsigned char * GrayBuf;
#endif

void print_time_statistics()
{
     lastsec = tv.tv_sec;
     cout << " sec=" << lastsec%60 << " ";
     int a = abs((int)frame_counter - (int)last_frame_counter);
     if(a>0) cout << "Hz=" << a;
     last_frame_counter = frame_counter; 
     fflush(stdout);
}

#ifdef ALSO_GRAY_WINDOW
void thread_routine_for_gray_buffer()
{
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
}
#endif

int main(int argc, char **argv)
{
    cout << "SimpleV4L2 - tibaldi.amos@gmail.com - https://github.com/Amos-Tibaldi/SimpleV4L2" << endl;
    cout << "Try to comment out the line #define ALSO_GRAY_WINDOW" << endl;
    RgbBuf = (unsigned char*)memalign(16, XPIX*YPIX*3);
#ifdef ALSO_GRAY_WINDOW
    GrayBuf = (unsigned char*)memalign(16, XPIX*YPIX*3);
#endif

    SXRWindow::Init();

    SXRWindow w(XPIX, YPIX, "vid");
#ifdef ALSO_GRAY_WINDOW
    SXRWindow wg(XPIX, YPIX, "gray");
#endif

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
		gettimeofday(&tv, &tz);
		if(lastsec==-1)
		{
		   lastsec = tv.tv_sec;
		}
		else
		{
		  if(tv.tv_sec != lastsec)
		  {
		     print_time_statistics();
		  }
		}
		frame_counter++;
		cout << ((frame_counter%10==0)?"x":".");
		fflush(stdout);
		
#ifdef ALSO_GRAY_WINDOW
		thread threadObj(thread_routine_for_gray_buffer);
		threadObj.join();

		wg.UpdateWindowOnScreen(GrayBuf);
#endif
		w.UpdateWindowOnScreen(RgbBuf);
       }

       SXRWindow::ShortManagementRoutine();
    }

    v.StopStreaming();

    v.Reset();

    free(RgbBuf);
#ifdef ALSO_GRAY_WINDOW
    free(GrayBuf);
#endif

    return 0;
}


