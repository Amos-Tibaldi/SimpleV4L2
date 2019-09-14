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
#include <vector>
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
#include "CV.h"

using namespace SimpleV4L2Namespace;
using namespace XRTestNamespace::Windowing;
using namespace std;
using namespace CVEasy;

#define XPIX 1920
#define YPIX 1080

struct timeval tv;
struct timezone tz;
unsigned int lastsec = -1;
unsigned int last_frame_counter = 0;
unsigned int frame_counter = 0;

FrameUC * theCVFrame = 0;
FrameF * fHarris = 0;
FrameUC * fGray = 0;
FrameUC * fGray2 = 0;
#define MAX_HARRIS_CORNERS (10000)
vector<int > vCornerX(MAX_HARRIS_CORNERS);
vector<int > vCornerY(MAX_HARRIS_CORNERS);

void print_time_statistics()
{
     lastsec = tv.tv_sec;
     cout << " sec=" << lastsec%60 << " ";
     int a = abs((int)frame_counter - (int)last_frame_counter);
     if(a>0) cout << "Hz=" << a;
     last_frame_counter = frame_counter; 
     fflush(stdout);
}

void thread_routine_for_gray_buffer()
{
	try
	{
	  fGray->clear();
	  ColorToGray(theCVFrame, fGray);
	  fHarris->clear();
	  cornerHarris(fGray, fHarris, 0.04);
          FrameAbs(fHarris);
          fGray2->clear();
	  FrameFloatToUChar3Chan(fHarris, fGray2);
          int c = 0;
	  vCornerX.clear();
          vCornerY.clear();
	  for(int j=10; j<fGray2->height-10; j++)
           for(int i=10; i<fGray2->width-10; i++) 
#define HARRIS_THRESHOLD (200)
            if((fGray2->ucbuf[3*(j*fGray2->width+i)]>HARRIS_THRESHOLD)||(fGray2->ucbuf[3*(j*fGray2->width+i)+1]>HARRIS_THRESHOLD)||(fGray2->ucbuf[3*(j*fGray2->width+i)+2]>HARRIS_THRESHOLD))
            {
              if(c<MAX_HARRIS_CORNERS)
              {
               vCornerX[c] = i;
               vCornerY[c] = j;
              }
             c++;
            }
	  for(int i=0; i<(c<MAX_HARRIS_CORNERS?c:MAX_HARRIS_CORNERS); i++)
          {
             FrameUC3PutRedCross(vCornerX[i], vCornerY[i], fGray2);
          }
          //cout << "Num-Harris-corners=" << c << endl; fflush(stdout);
	}
	catch(...)
	{
		cout << "EXCEPTION IN THREAD ROUTINE!" << endl;
		fflush(stdout);
	}
}

int main(int argc, char **argv)
{
    cout << "SimpleV4L2 - tibaldi.amos@gmail.com - https://github.com/Amos-Tibaldi/SimpleV4L2" << endl;

    SXRWindow::Init();

    SXRWindow w(XPIX, YPIX, "vid");
    SXRWindow wg(XPIX, YPIX, "Harris");

    theCVFrame = new FrameUC(XPIX, YPIX, 0, 3);
    fHarris = new FrameF(XPIX, YPIX, 0, 1);
    fGray = new FrameUC(XPIX, YPIX, 0, 1);
    fGray2 = new FrameUC(XPIX, YPIX, 0, 3);

    CSimpleV4L2 v(XPIX, YPIX, 0);

    bool bRet = v.Init();

    if(!bRet)
    {
        return -1;
    }

    v.StartStreaming();

    for(; !SXRWindow::ShouldExit(); )
    {
        if(v.NextFrame(theCVFrame->ucbuf))
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
		

		thread threadObj(thread_routine_for_gray_buffer);
		threadObj.join();

		wg.UpdateWindowOnScreen(fGray2->ucbuf);


		w.UpdateWindowOnScreen(theCVFrame->ucbuf);
       }

       SXRWindow::ShortManagementRoutine();
    }

    v.StopStreaming();

    v.Reset();

    delete theCVFrame;
    theCVFrame = 0;
    delete fHarris;
    fHarris = 0;
    delete fGray;
    fGray = 0;
    delete fGray2;
    fGray2 = 0;

    return 0;
}




