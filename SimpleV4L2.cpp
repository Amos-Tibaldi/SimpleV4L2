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
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libv4l2.h>
#include <libv4lconvert.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

#include "SimpleV4L2.h"

using namespace SimpleV4L2Namespace;

int  ng_debug          = 0;

const int CSimpleV4L2::m_iNBuffers = 3;

#define VIDEO_FMT_COUNT	    19

static __u32 the_pixelformat[VIDEO_FMT_COUNT] = {
    0,
    V4L2_PIX_FMT_HI240,
    V4L2_PIX_FMT_GREY,
    V4L2_PIX_FMT_RGB555,
    V4L2_PIX_FMT_RGB565,
    V4L2_PIX_FMT_RGB555X,
    V4L2_PIX_FMT_RGB565X,
    V4L2_PIX_FMT_BGR24,
    V4L2_PIX_FMT_BGR32,
    V4L2_PIX_FMT_RGB24,
    0,
    0,
    0,
    V4L2_PIX_FMT_YUYV,
    V4L2_PIX_FMT_YUV422P,
    V4L2_PIX_FMT_YUV420,
    0,
    0,
    V4L2_PIX_FMT_UYVY
};

const unsigned int ng_vfmt_to_depth[] = {
    0,               /* unused   */
    8,               /* RGB8     */
    8,               /* GRAY8    */
    16,              /* RGB15 LE */
    16,              /* RGB16 LE */
    16,              /* RGB15 BE */
    16,              /* RGB16 BE */
    24,              /* BGR24    */
    32,              /* BGR32    */
    24,              /* RGB24    */
    32,              /* RGB32    */
    16,              /* LUT2     */
    32,              /* LUT4     */
    16,		     /* YUYV     */
    16,		     /* YUV422P  */
    12,		     /* YUV420P  */
    0,		     /* MJPEG    */
    0,		     /* JPEG     */
    16,		     /* UYVY     */
};

static int xioctl(int fd, int cmd, void *arg, int mayfail)
{
    int rc;
    rc = v4l2_ioctl(fd,cmd,arg);
    if (rc >= 0 && ng_debug < 2)
    {
        return rc;
    }
    if (mayfail && ((errno == EINVAL) || (errno == ENOTTY)) && ng_debug < 2)
    {
        return rc;
    }
    fprintf(stderr, ": %s\n", (rc >= 0) ? "ok" : strerror(errno));
    return rc;
}

int CSimpleV4L2::v4l2_waiton()
{
    try
    {
        struct v4l2_buffer buf;
        struct timeval tv;
        fd_set rdset;

        /* wait for the next frame */
again:
        tv.tv_sec  = 5;
        tv.tv_usec = 0;
        FD_ZERO(&rdset);
        FD_SET(m_ilibv4l2_fd, &rdset);
        switch (select(m_ilibv4l2_fd + 1, &rdset, NULL, NULL, &tv)) {
        case -1:
            if (EINTR == errno)
                goto again;
            perror("v4l2: select");
            return -1;
        case  0:
            fprintf(stderr,"v4l2: oops: select timeout\n");
            return -1;
        }

        /* get it */
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (-1 == xioctl(m_ilibv4l2_fd, VIDIOC_DQBUF, &buf, 0))
        {
            return -1;
        }
        waiton++;
        m_Bufs[buf.index] = buf;

        return buf.index;
    }
    catch(...)
    {
        // LOG...
        return -1;
    }
} // END - int CSimpleV4L2::v4l2_waiton()

void ng_waiton_video_buf(struct ng_video_buf *buf)
{
    pthread_mutex_lock(&buf->lock);
    while (buf->refcount)
        pthread_cond_wait(&buf->cond, &buf->lock);
    pthread_mutex_unlock(&buf->lock);
}

int CSimpleV4L2::v4l2_queue_buffer()
{
    try
    {
        int frame = queue % m_ReqBufs.count;
        int rc;

        if (0 != m_BufMe[frame].refcount) {
            if (0 != queue - waiton)
            {
                return -1;
            }
            fprintf(stderr,"v4l2: waiting for a free buffer\n");
            ng_waiton_video_buf(m_BufMe+frame);
        }

        rc = xioctl(m_ilibv4l2_fd, VIDIOC_QBUF, &m_Bufs[frame], 0);
        if (0 == rc)
        {
            queue++;
        }
        return rc;
    }
    catch(...)
    {
        // LOG...
        return -1;
    }
}  // END - int CSimpleV4L2::v4l2_queue_buffer()

int64_t ng_tofday_to_timestamp(struct timeval *tv)
{
    long long ts;

    ts  = tv->tv_sec;
    ts *= 1000000;
    ts += tv->tv_usec;
    ts *= 1000;
    return ts;
}

bool CSimpleV4L2::NextFrame(unsigned char * RGBBuf)
{
    try
    {
        struct ng_video_buf *buf = NULL;
        int rc, size, frame = 0;

        v4l2_queue_all();
        frame = v4l2_waiton();
        if (-1 == frame)
        {
            return false;
        }
        m_BufMe[frame].refcount++;
        buf = &m_BufMe[frame];
        memset(&buf->info,0,sizeof(buf->info));
        buf->info.ts = ng_tofday_to_timestamp(&m_Bufs[frame].timestamp);

        if(m_FmtMe.fmtid == 9)
        {
            memcpy((void*)RGBBuf, (void*)buf->data, m_xwidth*m_yheight*3);
        }
        buf->refcount--;

        return true;
    }
    catch(...)
    {
        // LOG...
        return false;
    }
} // END - bool CSimpleV4L2::NextFrame(unsigned char * RGBBuf)


void CSimpleV4L2::v4l2_queue_all()
{
    try
    {
        for (;;) {
            if (queue - waiton >= m_ReqBufs.count)
            {
                return;
            }
            if (0 != v4l2_queue_buffer())
            {
                return;
            }
        }
    }
    catch(...)
    {
        // LOG...
    }
} // END - void CSimpleV4L2::v4l2_queue_all()

void CSimpleV4L2::StopStreaming()
{
    try
    {
        unsigned int i;

        if (-1 == v4l2_ioctl(m_ilibv4l2_fd, VIDIOC_STREAMOFF, &m_fmt_v4l2.type))
        {
            perror("ioctl VIDIOC_STREAMOFF");
        }

        for (i = 0; i < m_ReqBufs.count; i++)
        {
            if (0 != m_BufMe[i].refcount)
            {
                ng_waiton_video_buf(&m_BufMe[i]);
            }
            if (-1 == v4l2_munmap(m_BufMe[i].data, m_BufsSize[i]))
            {
                perror("munmap");
            }
        }
        queue = 0;
        waiton = 0;

        m_ReqBufs.count = 0;
        xioctl(m_ilibv4l2_fd, VIDIOC_REQBUFS, &m_ReqBufs, 1);
    }
    catch(...)
    {
        // LOG...
    }
} // END - void CSimpleV4L2::StopStreaming()

int CSimpleV4L2::StartStreaming()
{
    try
    {
        m_ReqBufs.count  = m_iNBuffers;
        m_ReqBufs.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        m_ReqBufs.memory = V4L2_MEMORY_MMAP;
        if (-1 == xioctl(m_ilibv4l2_fd, VIDIOC_REQBUFS, &m_ReqBufs, 0))
        {
            return -1;
        }
        for (unsigned int i = 0; i < m_ReqBufs.count; i++)
        {
            m_Bufs[i].index  = i;
            m_Bufs[i].type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            m_Bufs[i].memory = V4L2_MEMORY_MMAP;
            if (-1 == xioctl(m_ilibv4l2_fd, VIDIOC_QUERYBUF, &m_Bufs[i], 0))
            {
                return -1;
            }
            m_BufsSize[i] = m_Bufs[i].length;
            m_BufMe[i].fmt  = m_FmtMe;
            m_BufMe[i].size = m_BufMe[i].fmt.bytesperline * m_BufMe[i].fmt.height;
            m_BufMe[i].data = (unsigned char *) v4l2_mmap(NULL, m_Bufs[i].length,
                              PROT_READ | PROT_WRITE, MAP_SHARED,
                              m_ilibv4l2_fd, m_Bufs[i].m.offset);
            if (MAP_FAILED == m_BufMe[i].data) {
                perror("mmap");
                return -1;
            }
        }

        v4l2_queue_all();

        if (-1 == xioctl(m_ilibv4l2_fd, VIDIOC_STREAMON, &m_fmt_v4l2.type, 0))
        {
            return -1;
        }
        return 0;
    }
    catch(...)
    {
        // LOG...
        return -1;
    }
    return -1;
} // END - int CSimpleV4L2::StartStreaming()

bool CSimpleV4L2::Init()
{
    try
    {
        this->Reset();

        queue = 0;
        waiton = 0;

        std::string strDeviceNodeName = std::string("/dev/video");
        std::ostringstream oss;
        oss << m_iNDeviceNode;
        strDeviceNodeName.append(oss.str());

        if (-1 == (m_iFileDescriptor = open(strDeviceNodeName.c_str(), O_RDWR)))
        {
            // LOG error...
            this->Reset();
            return false;
        }

        if(-1 == (m_ilibv4l2_fd = v4l2_fd_open(m_iFileDescriptor, 0)))
        {
            // LOG error...
            this->Reset();
            return false;
        }

        struct ng_video_fmt fmt;
        fmt.fmtid = 9;
        fmt.width = m_xwidth;
        fmt.height = m_yheight;
        fmt.bytesperline = m_xwidth * 3;

        if(!SetFormat(&fmt))
        {
            //LOG...
            this->Reset();
            return false;
        }

        m_FmtMe = fmt;

        return true;
    }
    catch(...)
    {
        // LOG error...
        this->Reset();
        return false;
    }
} // END - bool CSimpleV4L2::Init()

bool CSimpleV4L2::SetFormat(struct ng_video_fmt * fmt)
{
    try
    {
        m_fmt_v4l2.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        m_fmt_v4l2.fmt.pix.pixelformat  = the_pixelformat[fmt->fmtid];
        m_fmt_v4l2.fmt.pix.width        = fmt->width;
        m_fmt_v4l2.fmt.pix.height       = fmt->height;
        m_fmt_v4l2.fmt.pix.field        = V4L2_FIELD_ANY;
        if (fmt->bytesperline != fmt->width * ng_vfmt_to_depth[fmt->fmtid]/8)
        {
            m_fmt_v4l2.fmt.pix.bytesperline = fmt->bytesperline;
        }
        else
        {
            m_fmt_v4l2.fmt.pix.bytesperline = 0;
        }


        if (-1 == xioctl(m_ilibv4l2_fd, VIDIOC_S_FMT, &m_fmt_v4l2, 1))
        {
            // LOG error...
            return false;
        }
        if (m_fmt_v4l2.fmt.pix.pixelformat != the_pixelformat[fmt->fmtid])
        {
            // LOG error...
            return false;
        }
        fmt->width        = m_fmt_v4l2.fmt.pix.width;
        fmt->height       = m_fmt_v4l2.fmt.pix.height;
        fmt->bytesperline = m_fmt_v4l2.fmt.pix.bytesperline;
        switch (fmt->fmtid) {
        case 14:
            fmt->bytesperline *= 2;
            break;
        case 15:
            fmt->bytesperline = fmt->bytesperline * 3 / 2;
            break;
        }
        if (0 == fmt->bytesperline)
        {
            fmt->bytesperline = fmt->width * ng_vfmt_to_depth[fmt->fmtid] / 8;
        }
        return true;
    }
    catch(...)
    {
        //LOG...
        return false;
    }
    return false;
}  // END - bool CSimpleV4L2::SetFormat(struct ng_video_fmt * fmt)

void CSimpleV4L2::Reset()
{
    try
    {
        if(m_ilibv4l2_fd != -1)
        {
            v4l2_close(m_ilibv4l2_fd);
        }

        if(m_iFileDescriptor != -1)
        {
            close(m_iFileDescriptor);
        }

    }
    catch(...)
    {
        // LOG...
    }
} // END - void CSimpleV4L2::Reset()

CSimpleV4L2::CSimpleV4L2(int xwidth, int yheight, int nDeviceNode)
{
    try
    {
        m_ilibv4l2_fd = -1;
        m_iFileDescriptor = -1;

        m_xwidth = xwidth;
        m_yheight = yheight;
        m_iNDeviceNode = nDeviceNode;
        memset(&m_ReqBufs, 0, sizeof(m_ReqBufs));
    }
    catch(...)
    {
        // LOG...
    }
}  // END - CSimpleV4L2::CSimpleV4L2(int xwidth, int yheight, int nDeviceNode)

CSimpleV4L2::~CSimpleV4L2()
{
    try
    {

    }
    catch(...)
    {
        // LOG...
    }
} // END - CSimpleV4L2::~CSimpleV4L2()







