/*
	Copyright 2021 flyinghead

	This file is part of Flycast.

    Flycast is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Flycast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Flycast.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once
#if defined(LIBRETRO) && (defined(HAVE_OPENGL) || defined(HAVE_OPENGLES))
#if defined(TARGET_IPHONE) //apple-specific ogles3 headers
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#endif
#include "gl_context.h"
#include <libretro.h>
#include <glsm/glsm.h>
#include <glsm/glsmsym.h>

#ifdef glEnable
#undef glEnable

inline void rglEnableExt(GLenum cap)
{
#ifdef GL_PRIMITIVE_RESTART
	if (cap == SGL_PRIMITIVE_RESTART)
	{
#ifdef GLSM_DEBUG
		log_cb(RETRO_LOG_INFO, "glEnable.\n");
#endif
		glsm_ctl(GLSM_CTL_IMM_VBO_DRAW, NULL);
		glEnable(GL_PRIMITIVE_RESTART);
		return;
	}
#endif

#ifdef GL_PRIMITIVE_RESTART_FIXED_INDEX
	if (cap == SGL_PRIMITIVE_RESTART_FIXED_INDEX)
	{
#ifdef GLSM_DEBUG
		log_cb(RETRO_LOG_INFO, "glEnable.\n");
#endif
		glsm_ctl(GLSM_CTL_IMM_VBO_DRAW, NULL);
		glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
		return;
	}
#endif

	rglEnable(cap);
}

#define glEnable(T) rglEnableExt(S##T)
#endif

#ifndef GL_MAJOR_VERSION
#define GL_MAJOR_VERSION                  0x821B
#endif
#ifndef GL_MINOR_VERSION
#define GL_MINOR_VERSION                  0x821C
#endif

#ifndef GL_DEBUG_SEVERITY_NOTIFICATION
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B
#endif
#ifndef GL_DEBUG_SEVERITY_HIGH
#define GL_DEBUG_SEVERITY_HIGH            0x9146
#endif
#ifndef GL_DEBUG_SEVERITY_MEDIUM
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#endif
#ifndef GL_DEBUG_SEVERITY_LOW
#define GL_DEBUG_SEVERITY_LOW             0x9148
#endif

class LibretroGraphicsContext : public GLGraphicsContext
{
public:
	bool init() {
		postInit();
		return true;
	}

	void term() override {
		preTerm();
	}

	void swap() {}
};

extern LibretroGraphicsContext theGLContext;
#endif
