#-----------------------------------------------------------------------
#
# This file is part of the SimpleV4L2 Project
#
#  by Amos Tibaldi - tibaldi at users.sourceforge.net
#
# http://sourceforge.net/projects/simplev4l2/
#
# http://simplev4l2.sourceforge.net/
#
#
# COPYRIGHT: http://www.gnu.org/licenses/gpl.html
#            COPYRIGHT-gpl-3.0.txt
#
#     The SimpleV4L2 Project
#         Simple V4L2 Grabber and RGB Buffer SSSE3 Optimized Video Player
#
#     Copyright (C) 2013 Amos Tibaldi
#
#     This program is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.
#
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#-----------------------------------------------------------------------

project(simplev4l2)
set(CMAKE_CXX_FLAGS "-std=c++11 -msse3 -mssse3 -fpermissive -g")
add_executable(simplev4l2 main.cpp SimpleV4L2.cpp SSSE3RgbToRgba.cpp SXRWindow.cpp CV.cpp)

target_link_libraries(simplev4l2 pthread v4l2 X11 Xrender)
