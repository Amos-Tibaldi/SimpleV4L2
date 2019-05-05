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

#ifndef SIMPLEV4L2_H
#define SIMPLEV4L2_H

#define SIMPLEV4L2_MAX_NBUFFERS 30

namespace SimpleV4L2Namespace
{

struct ng_video_fmt {
    unsigned int   fmtid;
    unsigned int   width;
    unsigned int   height;
    unsigned int   bytesperline;
};

struct ng_video_buf {
    struct ng_video_fmt  fmt;
    size_t               size;
    unsigned char        *data;
    struct {
        int64_t          ts;
        int              seq;
        int              twice;
    } info;
    pthread_mutex_t      lock;
    pthread_cond_t       cond;
    int                  refcount;
    void                 (*release)(struct ng_video_buf *buf);
    void                 *priv;
};


class CSimpleV4L2
{
public:
    CSimpleV4L2(int xwidth, int yheight, int nDeviceNode = 0);
    ~CSimpleV4L2();

    bool Init();
    void Reset();

    int StartStreaming();
    void StopStreaming();
    bool NextFrame(unsigned char * RGBBuf);

private:
    int m_iNDeviceNode;
    int m_iFileDescriptor;
    int m_ilibv4l2_fd;
    int m_xwidth;
    int m_yheight;
    static const int m_iNBuffers;

    struct v4l2_requestbuffers m_ReqBufs;
    struct v4l2_buffer m_Bufs[SIMPLEV4L2_MAX_NBUFFERS];
    int m_BufsSize[SIMPLEV4L2_MAX_NBUFFERS];
    struct ng_video_buf m_BufMe[SIMPLEV4L2_MAX_NBUFFERS];
    unsigned int queue;
    unsigned int waiton;
    struct ng_video_fmt m_FmtMe;

    struct v4l2_format             m_fmt_v4l2;

    int v4l2_queue_buffer();
    void v4l2_queue_all();
    bool SetFormat(struct ng_video_fmt * fmt);
    int v4l2_waiton();
};



} // END NAMESPACE SimpleV4L2Namespace



#endif

