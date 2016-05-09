//-----------------------------------------------------------------------//
//A GPU based particle engine                                            //
//Copyright (C) 2013 Joseph Dillon                                       //
//-----------------------------------------------------------------------//
//This program is free software: you can redistribute it and/or modify   //
//it under the terms of the GNU General Public License as published by   //
//the Free Software Foundation, either version 3 of the License, or      //
//(at your option) any later version.                                    //
//                                                                       //
//This program is distributed in the hope that it will be useful,        //
//but WITHOUT ANY WARRANTY; without even the implied warranty of         //
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          //
//GNU General Public License for more details.                           //
//                                                                       //
//You should have received a copy of the GNU General Public License      //
//along with this program.  If not, see <http://www.gnu.org/licenses/>.  //
//-----------------------------------------------------------------------//
#include "frame.hpp"

#include "util.hpp"

#include <iostream>
#include <iomanip>
#include <ctime>

Frame::Frame(ParticleShaders& shaders, cl::Context context, cl::Device device)
  : ps(context, device, shaders)
{
  srand((unsigned)time(0));
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  glPointSize(1);
}

void Frame::Reshape(int width, int height)
{
  this->width = width;
  this->height = height;
  glViewport(0, 0, width, height);
}

void Frame::MouseMove(float x, float y)
{
  ps.UpdateForceLocation(0,
                          -1.0f + ((x / width)  * 2.0f),
                         (-1.0f + ((y / height) * 2.0f)) * -1);
  ps.FlushCL();
}

void Frame::MouseClick(float click)
{
  ps.UpdateForcePower(0,
                      click);
  ps.FlushCL();
}

static int num = 0;

void Frame::Render()
{
  glFinish();
  
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  ps.Render();
}
