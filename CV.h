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

#ifndef CVH
#define CVH



namespace CVEasy
{

	class FrameUC
	{
		public:
		FrameUC(int w, int h, unsigned char * bufsrc, int _channels);
		~FrameUC();
		void clear();
		int width;
		int height;
		int channels;
		int iNumPixels;
		unsigned char * ucbuf;

	};

	class FrameF
	{
		public:
		FrameF(int w, int h, float * bufsrc, int _channels);
		~FrameF();
		void clear();
		int width;
		int height;
		int channels;
		int iNumPixels;
		float * fbuf;

	};

	void FrameUC3PutRedCross(int x, int y, FrameUC * f);
	void cornerHarris(FrameUC * src, FrameF * eigenv, double k);
	void ColorToGray(FrameUC * f, FrameUC * g);
	void FrameFloatToUChar3Chan(FrameF * f, FrameUC * fout);
	void PrintFrameFloat(FrameF * f);
	void FrameAbs(FrameF * f);


}




#endif


