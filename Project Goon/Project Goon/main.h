#pragma once

#include <glm/glm.hpp>
using namespace glm;

extern mat4 Projection;
extern mat4 View;
extern double delta;
extern mat4 depthBiasMVP;
extern GLuint depthTexture;
extern Program shadow_program;