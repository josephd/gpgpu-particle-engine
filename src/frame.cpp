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
#include "particle_hex.hpp"

#include <iostream>

Frame::Frame()
{
  num_particles = 10;
}

Frame::~Frame()
{
}

void Frame::Reshape(int width, int height)
{
  glViewport(0, 0, width, height);
}

void Frame::Init(cl::Context context, cl::Device device)
{
  sprite = loadTexture("media/textures/particle.png", true);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPointSize(8);

  GLuint vert = makeShader(GL_VERTEX_SHADER,   1, { new std::string("media/shaders/particle.vert.glsl") });
  GLuint frag = makeShader(GL_FRAGMENT_SHADER, 1, { new std::string("media/shaders/particle.frag.glsl") });

  prog = makeProgram(vert, frag);

  glUseProgram(prog);

  GLint pointsprite = glGetUniformLocation(prog, "pointsprite");

  glUniform1i(pointsprite, 0);
  glActiveTexture(GL_TEXTURE0 + 0);
  glBindTexture(GL_TEXTURE_2D, sprite);

  GLint position = glGetAttribLocation(prog, "position");
  GLint color    = glGetAttribLocation(prog, "color");

  GLfloat *particlesv = new GLfloat[num_particles * 2];
  unsigned j = 0;
  for (unsigned i = 0; i < num_particles; ++i)
  {
    particlesv[j++] = 0;
    particlesv[j++] = 0;
  }

  GLfloat *particlesc = new GLfloat[num_particles * 3];
  j = 0;
  for (unsigned i = 0; i < num_particles; ++i)
  {
    particlesc[j++] = 0.0f;
    particlesc[j++] = 1.0f;
    particlesc[j++] = 0.0f;
  }

  float *particles_vel = new float[num_particles * 2];
  j = 0;
  for (unsigned i = 0; i < num_particles; ++i)
  {
    particlesc[j++] = 0.01f;
    particlesc[j++] = 0.01f;
  }

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbov);
  glBindBuffer(GL_ARRAY_BUFFER, vbov);
  glBufferData(GL_ARRAY_BUFFER, num_particles * 2 * sizeof(GLfloat), particlesv, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(position);
  glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

  glGenBuffers(1, &vboc);
  glBindBuffer(GL_ARRAY_BUFFER, vboc);
  glBufferData(GL_ARRAY_BUFFER, num_particles * 3 * sizeof(GLfloat), particlesc, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(color);
  glVertexAttribPointer(color, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

  vbo.push_back(cl::BufferGL(context, CL_MEM_READ_WRITE, vbov));
  vbo.push_back(cl::BufferGL(context, CL_MEM_READ_WRITE, vboc));
  cl::Buffer veloBuffer(context, CL_MEM_READ_WRITE, num_particles, NULL);

  glBindFragDataLocation(prog, 0, "colorOut");

  glFinish();

  cl::Program::Sources sources;

  std::string kernel(reinterpret_cast<const char*>(particle_cl), particle_cl_len);
  sources.push_back({ kernel.c_str(), kernel.size() });
  cl::Program program(context, sources);

  std::vector<cl::Device> devices;
  devices.push_back(device);
  if (program.build(devices) != CL_SUCCESS)
  {
    std::cerr << "CL build error: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;
    exit(EXIT_FAILURE);
  }

  queue = cl::CommandQueue(context, device);
  std::cout << queue.enqueueWriteBuffer(veloBuffer, CL_TRUE, 0, num_particles * 2 * sizeof(float), particles_vel, NULL) << std::endl;
  queue.finish();

  k_particlePhysics = cl::Kernel(program, "particlePhysics");
  k_particlePhysics.setArg(0, vbov);
  k_particlePhysics.setArg(1, veloBuffer);
  k_particlePhysics.setArg(2, vboc);
  queue.finish();
}

void Frame::Render()
{
  glFinish();
  return;
  std::cout << queue.enqueueAcquireGLObjects(&vbo, NULL) << std::endl;
  queue.finish();
  std::cout << queue.enqueueNDRangeKernel(k_particlePhysics, cl::NullRange, cl::NDRange(num_particles), cl::NullRange, NULL) << std::endl;
  queue.finish();
  std::cout << queue.enqueueReleaseGLObjects(&vbo, NULL) << std::endl;
  queue.finish();

  glClear(GL_COLOR_BUFFER_BIT);

  glBindTexture(GL_TEXTURE_2D, sprite);

  glUseProgram(prog);
  glBindVertexArray(vao);
  glDrawArrays(GL_POINTS, 0, num_particles);
}

void Frame::Destroy()
{
  glDeleteTextures(1, &sprite);
}