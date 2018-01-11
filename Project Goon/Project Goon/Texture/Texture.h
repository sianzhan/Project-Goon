#pragma once
#include <GL/glew.h>
#include <string>
#include <vector>


namespace Texture {
	bool	LoadPngImage(const char *name, int &outWidth, int &outHeight, bool &outHasAlpha, GLubyte **outData);
	GLuint	GenTexture(const char* filepath);
	void ScreenShot(std::string& file);
}