#ifndef SP_GRAPHICS_OPENGL_DESKTOP_H
#define SP_GRAPHICS_OPENGL_DESKTOP_H

#ifndef SP_GRAPHICS_OPENGL_H
#error "This file should only be included from <graphics/opengl.h>"
#endif

/**
    The following are functions and options not supported by OpenGLES2, but needed for desktop OpenGL compatibility.
    As they will provide more optimal solutions, or better compatibility with desktop OpenGL.
    
    This set should be as minimal as possible, as any function here needs extra compatibility work with OpenGLES2
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef EMSCRIPTEN
#define GL_DEPTH_STENCIL 0x84f9
#else
#define GL_DEPTH24_STENCIL8 0x88F0
#endif
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A

#ifdef __cplusplus
}//extern "C"
#endif

#endif//SP2_GRAPHICS_OPENGL_DESKTOP_H
