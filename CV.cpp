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

#include "CV.h"

using namespace std;

namespace CVEasy
{

FrameUC::FrameUC(int w, int h, unsigned char * bufsrc, int _channels)
{
	iNumPixels = w * h;
	width = w;
	height = h;
	channels = _channels;
	int nBytes = iNumPixels*channels;
	ucbuf = (unsigned char*) memalign(16, nBytes);
	if(bufsrc != 0)
	{
		memcpy(ucbuf, bufsrc, nBytes);
	}
	else
	{
		memset(ucbuf, 0, nBytes);
	}
}

FrameUC::~FrameUC()
{
	width = height = channels = iNumPixels = -1;
	free(ucbuf);
	ucbuf = 0;
}

void FrameUC::clear()
{
   memset(ucbuf, 0, iNumPixels*channels);
}

FrameF::FrameF(int w, int h, float * bufsrc, int _channels)
{
	iNumPixels = w * h;
	width = w;
	height = h;
	channels = _channels;
	int nBytes = sizeof(float)*iNumPixels*channels;
	fbuf = (float*) malloc(nBytes);
	if(bufsrc != 0)
	{
		memcpy(fbuf, bufsrc, nBytes);
	}
	else
	{
		memset(fbuf, 0, nBytes);
	}
}

FrameF::~FrameF()
{
	width = height = channels = iNumPixels = -1;
	free(fbuf);
	fbuf = 0;
}

void FrameF::clear()
{
   memset(fbuf, 0, sizeof(float)*iNumPixels*channels);
}


void calcHarris(FrameF * _cov , FrameF * _dst, double k)
{
    int i, j;
    for( i = 0; i < _cov->height; i++ )
    {
        const float* cov = &(_cov->fbuf[3*i*_cov->width]);
        float* dst = &(_dst->fbuf[i*_dst->width]);
        for(j = 0 ; j < _cov->width; j++ )
        {
            float a = cov[j*3];
            float b = cov[j*3+1];
            float c = cov[j*3+2];
            dst[j] = (float)(a*c - b*b - k*(a + c)*(a + c));
        }
    }
}

void Sobel(FrameUC * src, FrameF * dst, bool dx)
{
    if((src->channels != 1)||(dst->channels != 1))
    {
      return;
    }
    if((src->width!=dst->width)||(src->height!=dst->height))
    { 
      return;
    }
    for(int j=1; j<src->height-1; j++)
    {
	for(int i=1; i<src->width-1; i++)
	{
		if(!dx)
		{
			int yoffmo = (j-1)*src->width;
			int yoffpo = (j+1)*src->width;
			unsigned char uc1 = (src->ucbuf[yoffmo+(i-1)]);
                        unsigned char uc2 = (src->ucbuf[yoffmo+(i)]);
                        unsigned char uc3 = (src->ucbuf[yoffmo+(i+1)]);
			unsigned char uc4 = (src->ucbuf[yoffpo+(i-1)]);
                        unsigned char uc5 = (src->ucbuf[yoffpo+(i)]);
                        unsigned char uc6 = (src->ucbuf[yoffpo+(i+1)]);
			dst->fbuf[j*src->width+i] = (float)(-uc1-2*uc2-uc3+uc4+2*uc5+uc6);
		}
		else
		{
			float a = -(src->ucbuf[((j-1)*src->width)+(i-1)])
                                  -2*(src->ucbuf[((j)*src->width)+(i-1)])
                                  -(src->ucbuf[((j+1)*src->width)+(i-1)])
				  +(src->ucbuf[((j-1)*src->width)+(i+1)])
                                  +2*(src->ucbuf[((j)*src->width)+(i+1)])
                                  +(src->ucbuf[((j+1)*src->width)+(i+1)]);
			dst->fbuf[j*dst->width+i] = a;
		}
	}
    }
}

void cornerHarris(FrameUC * src, FrameF * eigenv, double k)
{
    if(src->channels != 1)
    {
       return;
    }
    FrameF * Dx = new FrameF(src->width, src->height, 0, 1);
    FrameF * Dy = new FrameF(src->width, src->height, 0, 1);
    Sobel(src, Dx, true);
    Sobel(src, Dy, false);
    FrameF * cov = new FrameF( src->width, src->height, 0, 3);
    int i, j;
    for( i = 0; i < src->height; i++ )
    {
        float* cov_data = &(cov->fbuf[3*i*cov->width]);
        const float* dxdata = &(Dx->fbuf[i*Dx->width]);
        const float* dydata = &(Dy->fbuf[i*Dy->width]);
        for(j = 0; j < src->width; j++ )
        {
            float dx = dxdata[j];
            float dy = dydata[j];
            cov_data[j*3] = dx*dx;
            cov_data[j*3+1] = dx*dy;
            cov_data[j*3+2] = dy*dy;
        }
    }
    calcHarris(cov, eigenv, k);
    delete cov;
    delete Dy;
    delete Dx;
}

void PrintFrameFloat(FrameF * f)
{
	for(int j=0; j<f->height; j++)
	{
		for(int i=0; i<f->width; i++)
		{
			for(int c=0; c<f->channels; c++)
			{
			 cout << "[" <<(int)i<<";"<<(int)j<<";"<<(int)c<<"] "<< (float)f->fbuf[f->channels*(i+j*f->width)+c]<< " " ;fflush(stdout);
                        }
                }
                cout <<endl;
        }
}

void ColorToGray(FrameUC * f, FrameUC * g)
{
   try
   {
	    if((f->channels != 3)||(!((g->channels==1)||(g->channels==3))))
	    {
		return;
	    }
	    bool bok = false;
	    if(g->channels==3)
	    {
              bok = true;
            }
	    unsigned char * srcoff = f->ucbuf;
	    unsigned char * dstoff = g->ucbuf;
	    for(int j=0; j<f->height; j++)
	    {
	      for(int i=0; i<f->width; i++)
	      {
		unsigned int mean = *srcoff++;
		mean += *srcoff++;
		mean += *srcoff++;
		mean /=3;
		mean = (mean<0?0:mean);
		mean = (mean>255?255:mean);
		unsigned char ucmean = mean;
		*dstoff++ = ucmean;
		if(bok)
		{
		    *dstoff++ = ucmean;
		    *dstoff++ = ucmean;
                }
	      }
	    }

   }catch(...)
   {
	cout << "Exception in ColorToGray!" <<endl;
   }
}

void FrameUC3PutRedCross(int x, int y, FrameUC * f)
{
   if(f->channels != 3) return;
   for(int i=-20; i<20; i++)
   {
     if((x+i>=0)&&(x+i<f->width))
       f->ucbuf[(y*f->width+x+i)*3]=255;
     if((i+y>=0)&&(i+y<f->height))
       f->ucbuf[((i+y)*f->width+x)*3] = 255;
   }
}

void FrameFloatToUChar3Chan(FrameF * f, FrameUC * fout)
{
        int fchan = f->channels;
        if((fchan != 1)&&(fchan != 3))
        {
           return;
        }
        if(fout->channels != 3) return;
	float fmax = numeric_limits<float>::min();
	float fmin = numeric_limits<float>::max();
	float * pf = f->fbuf;
	for(int j=0; j<f->height; j++)
	{
		for(int i=0; i<f->width; i++)
		{
                    for(int c=0; c<fchan; c++)
                    {
			float a = *pf++; 
			if(a<fmin) fmin = a;
			if(a>fmax) fmax = a;				
                    }
		}
	}
	float ratio = 255.0f / (fmax-fmin);
	if(ratio!=ratio)
	{
		cout << "fmax=fmin, null denominator" << endl; fflush(stdout);
	}
	//cout << "ratio=" << (float) ratio << " fmax=" << (float)fmax << " fmin="<<(float)fmin<<endl;
	unsigned char * unpunt = fout->ucbuf;
	float * pfin = f->fbuf;
	for(int j=0; j<f->height; j++)
	{
		for(int i=0; i<f->width; i++)
		{
                    if(fchan==1)
                    {
			float a = *pfin++;
			unsigned char uca = (unsigned char) ((float)((a-fmin) * ratio));
			*unpunt++ = uca;
			*unpunt++ = uca;
			*unpunt++ = uca;
                    }
                    if(fchan==3)
                    {
                        for(int h=0; h<fchan; h++)
                        {
		                float a = *pfin++;
		                unsigned char uca = (unsigned char) ((float)((a-fmin) * ratio));
		                *unpunt++ = uca;
                        }
                    }
		}
	}
	/*
	unsigned char ucmax = 0;
        unsigned char ucmin = 255;
	unsigned char * puc = fout->ucbuf;
	for(int j=0; j<f->height; j++)
	{
		for(int i=0; i<f->width; i++)
		{
			for(int h=0; h<fchan; h++)
			{
				unsigned char uc = *puc++;
				if(uc>ucmax) ucmax = uc;
				if(uc<ucmin) ucmin = uc;
			}
		}
	}
	cout << "ucmax=" <<(int)ucmax<<" ucmin=" <<(int)ucmin <<endl; fflush(stdout);
	*/
}

void FrameAbs(FrameF * f)
{
	int nChan = f->channels;
	float * psrc = f->fbuf;
	float * pdst = f->fbuf;
	for(int j=0; j<f->height; j++)
	{
		for(int i=0; i<f->width; i++)
		{
			for(int c=0; c<nChan; c++)
			{
				float a = *psrc++;
				if(a<0) *pdst++ = -a;
				else *pdst++ = a;
			}
		}
	}
}


}


