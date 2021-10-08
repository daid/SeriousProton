#ifndef SP_GRAPHICS_OPENGL_H
#define SP_GRAPHICS_OPENGL_H

#ifdef __WIN32
#define GL_APIENTRY __stdcall
#else
#define GL_APIENTRY
#endif

#include <graphics/opengles2.h>
#include <graphics/openglDesktop.h>

namespace sp {
void initOpenGL();
}//namespace sp


#ifdef DEBUG
#define SP_ENABLE_OPENGL_TRACING
#endif
#ifdef SP_ENABLE_OPENGL_TRACING
#include <stringImproved.h>
namespace sp {
void traceOpenGLCall(const char* function_name, const char* source_file, const char* source_function, int source_line_number, const string& parameters);
static inline string traceOpenGLCallParams() { return ""; }
template<typename T> std::enable_if_t<std::is_integral_v<T>, string> traceOpenGLCallParam(T n) { return string(static_cast<int>(n)); }
template<typename T> std::enable_if_t<std::is_floating_point_v<T>, string> traceOpenGLCallParam(T n) { return string(static_cast<float>(n)); }
template<typename T> std::enable_if_t<std::is_pointer_v<T> || std::is_null_pointer_v<T>, string> traceOpenGLCallParam(T) { return "[ptr]"; }
template<typename A1> string traceOpenGLCallParams(A1&& a) { return traceOpenGLCallParam(std::forward<A1>(a)); }
template<typename A1, typename... ARGS> string traceOpenGLCallParams(A1&& a, ARGS&&... args) { return traceOpenGLCallParam(std::forward<A1>(a)) + ", " + traceOpenGLCallParams(std::forward<ARGS>(args)...); }
}//namespace sp

#ifdef _MSC_VER
#define SP_PRETTY_FUNCTION __FUNCTION__
#else
#define SP_PRETTY_FUNCTION __PRETTY_FUNCTION__
#endif

#define glActiveTexture(...) do { glActiveTexture(__VA_ARGS__); sp::traceOpenGLCall("glActiveTexture", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glAttachShader(...) do { glAttachShader(__VA_ARGS__); sp::traceOpenGLCall("glAttachShader", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glBindAttribLocation(...) do { glBindAttribLocation(__VA_ARGS__); sp::traceOpenGLCall("glBindAttribLocation", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glBindBuffer(...) do { glBindBuffer(__VA_ARGS__); sp::traceOpenGLCall("glBindBuffer", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glBindFramebuffer(...) do { glBindFramebuffer(__VA_ARGS__); sp::traceOpenGLCall("glBindFramebuffer", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glBindRenderbuffer(...) do { glBindRenderbuffer(__VA_ARGS__); sp::traceOpenGLCall("glBindRenderbuffer", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glBindTexture(...) do { glBindTexture(__VA_ARGS__); sp::traceOpenGLCall("glBindTexture", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glBlendColor(...) do { glBlendColor(__VA_ARGS__); sp::traceOpenGLCall("glBlendColor", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glBlendEquation(...) do { glBlendEquation(__VA_ARGS__); sp::traceOpenGLCall("glBlendEquation", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glBlendEquationSeparate(...) do { glBlendEquationSeparate(__VA_ARGS__); sp::traceOpenGLCall("glBlendEquationSeparate", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glBlendFunc(...) do { glBlendFunc(__VA_ARGS__); sp::traceOpenGLCall("glBlendFunc", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glBlendFuncSeparate(...) do { glBlendFuncSeparate(__VA_ARGS__); sp::traceOpenGLCall("glBlendFuncSeparate", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glBufferData(...) do { glBufferData(__VA_ARGS__); sp::traceOpenGLCall("glBufferData", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glBufferSubData(...) do { glBufferSubData(__VA_ARGS__); sp::traceOpenGLCall("glBufferSubData", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glClear(...) do { glClear(__VA_ARGS__); sp::traceOpenGLCall("glClear", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glClearColor(...) do { glClearColor(__VA_ARGS__); sp::traceOpenGLCall("glClearColor", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glClearDepthf(...) do { glClearDepthf(__VA_ARGS__); sp::traceOpenGLCall("glClearDepthf", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glClearStencil(...) do { glClearStencil(__VA_ARGS__); sp::traceOpenGLCall("glClearStencil", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glColorMask(...) do { glColorMask(__VA_ARGS__); sp::traceOpenGLCall("glColorMask", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glCompileShader(...) do { glCompileShader(__VA_ARGS__); sp::traceOpenGLCall("glCompileShader", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glCompressedTexImage2D(...) do { glCompressedTexImage2D(__VA_ARGS__); sp::traceOpenGLCall("glCompressedTexImage2D", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glCompressedTexSubImage2D(...) do { glCompressedTexSubImage2D(__VA_ARGS__); sp::traceOpenGLCall("glCompressedTexSubImage2D", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glCopyTexImage2D(...) do { glCopyTexImage2D(__VA_ARGS__); sp::traceOpenGLCall("glCopyTexImage2D", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glCopyTexSubImage2D(...) do { glCopyTexSubImage2D(__VA_ARGS__); sp::traceOpenGLCall("glCopyTexSubImage2D", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glCullFace(...) do { glCullFace(__VA_ARGS__); sp::traceOpenGLCall("glCullFace", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glDeleteBuffers(...) do { glDeleteBuffers(__VA_ARGS__); sp::traceOpenGLCall("glDeleteBuffers", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glDeleteFramebuffers(...) do { glDeleteFramebuffers(__VA_ARGS__); sp::traceOpenGLCall("glDeleteFramebuffers", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glDeleteProgram(...) do { glDeleteProgram(__VA_ARGS__); sp::traceOpenGLCall("glDeleteProgram", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glDeleteRenderbuffers(...) do { glDeleteRenderbuffers(__VA_ARGS__); sp::traceOpenGLCall("glDeleteRenderbuffers", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glDeleteShader(...) do { glDeleteShader(__VA_ARGS__); sp::traceOpenGLCall("glDeleteShader", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glDeleteTextures(...) do { glDeleteTextures(__VA_ARGS__); sp::traceOpenGLCall("glDeleteTextures", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glDepthFunc(...) do { glDepthFunc(__VA_ARGS__); sp::traceOpenGLCall("glDepthFunc", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glDepthMask(...) do { glDepthMask(__VA_ARGS__); sp::traceOpenGLCall("glDepthMask", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glDepthRangef(...) do { glDepthRangef(__VA_ARGS__); sp::traceOpenGLCall("glDepthRangef", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glDetachShader(...) do { glDetachShader(__VA_ARGS__); sp::traceOpenGLCall("glDetachShader", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glDisable(...) do { glDisable(__VA_ARGS__); sp::traceOpenGLCall("glDisable", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glDisableVertexAttribArray(...) do { glDisableVertexAttribArray(__VA_ARGS__); sp::traceOpenGLCall("glDisableVertexAttribArray", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glDrawArrays(...) do { glDrawArrays(__VA_ARGS__); sp::traceOpenGLCall("glDrawArrays", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glDrawElements(...) do { glDrawElements(__VA_ARGS__); sp::traceOpenGLCall("glDrawElements", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glEnable(...) do { glEnable(__VA_ARGS__); sp::traceOpenGLCall("glEnable", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glEnableVertexAttribArray(...) do { glEnableVertexAttribArray(__VA_ARGS__); sp::traceOpenGLCall("glEnableVertexAttribArray", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glFinish(...) do { glFinish(__VA_ARGS__); sp::traceOpenGLCall("glFinish", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glFlush(...) do { glFlush(__VA_ARGS__); sp::traceOpenGLCall("glFlush", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glFramebufferRenderbuffer(...) do { glFramebufferRenderbuffer(__VA_ARGS__); sp::traceOpenGLCall("glFramebufferRenderbuffer", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glFramebufferTexture2D(...) do { glFramebufferTexture2D(__VA_ARGS__); sp::traceOpenGLCall("glFramebufferTexture2D", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glFrontFace(...) do { glFrontFace(__VA_ARGS__); sp::traceOpenGLCall("glFrontFace", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGenBuffers(...) do { glGenBuffers(__VA_ARGS__); sp::traceOpenGLCall("glGenBuffers", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGenerateMipmap(...) do { glGenerateMipmap(__VA_ARGS__); sp::traceOpenGLCall("glGenerateMipmap", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGenFramebuffers(...) do { glGenFramebuffers(__VA_ARGS__); sp::traceOpenGLCall("glGenFramebuffers", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGenRenderbuffers(...) do { glGenRenderbuffers(__VA_ARGS__); sp::traceOpenGLCall("glGenRenderbuffers", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGenTextures(...) do { glGenTextures(__VA_ARGS__); sp::traceOpenGLCall("glGenTextures", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetActiveAttrib(...) do { glGetActiveAttrib(__VA_ARGS__); sp::traceOpenGLCall("glGetActiveAttrib", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetActiveUniform(...) do { glGetActiveUniform(__VA_ARGS__); sp::traceOpenGLCall("glGetActiveUniform", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetAttachedShaders(...) do { glGetAttachedShaders(__VA_ARGS__); sp::traceOpenGLCall("glGetAttachedShaders", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetBooleanv(...) do { glGetBooleanv(__VA_ARGS__); sp::traceOpenGLCall("glGetBooleanv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetBufferParameteriv(...) do { glGetBufferParameteriv(__VA_ARGS__); sp::traceOpenGLCall("glGetBufferParameteriv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetFloatv(...) do { glGetFloatv(__VA_ARGS__); sp::traceOpenGLCall("glGetFloatv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetFramebufferAttachmentParameteriv(...) do { glGetFramebufferAttachmentParameteriv(__VA_ARGS__); sp::traceOpenGLCall("glGetFramebufferAttachmentParameteriv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetIntegerv(...) do { glGetIntegerv(__VA_ARGS__); sp::traceOpenGLCall("glGetIntegerv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetProgramiv(...) do { glGetProgramiv(__VA_ARGS__); sp::traceOpenGLCall("glGetProgramiv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetProgramInfoLog(...) do { glGetProgramInfoLog(__VA_ARGS__); sp::traceOpenGLCall("glGetProgramInfoLog", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetRenderbufferParameteriv(...) do { glGetRenderbufferParameteriv(__VA_ARGS__); sp::traceOpenGLCall("glGetRenderbufferParameteriv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetShaderiv(...) do { glGetShaderiv(__VA_ARGS__); sp::traceOpenGLCall("glGetShaderiv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetShaderInfoLog(...) do { glGetShaderInfoLog(__VA_ARGS__); sp::traceOpenGLCall("glGetShaderInfoLog", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetShaderPrecisionFormat(...) do { glGetShaderPrecisionFormat(__VA_ARGS__); sp::traceOpenGLCall("glGetShaderPrecisionFormat", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetShaderSource(...) do { glGetShaderSource(__VA_ARGS__); sp::traceOpenGLCall("glGetShaderSource", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetTexParameterfv(...) do { glGetTexParameterfv(__VA_ARGS__); sp::traceOpenGLCall("glGetTexParameterfv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetTexParameteriv(...) do { glGetTexParameteriv(__VA_ARGS__); sp::traceOpenGLCall("glGetTexParameteriv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetUniformfv(...) do { glGetUniformfv(__VA_ARGS__); sp::traceOpenGLCall("glGetUniformfv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetUniformiv(...) do { glGetUniformiv(__VA_ARGS__); sp::traceOpenGLCall("glGetUniformiv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetVertexAttribfv(...) do { glGetVertexAttribfv(__VA_ARGS__); sp::traceOpenGLCall("glGetVertexAttribfv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetVertexAttribiv(...) do { glGetVertexAttribiv(__VA_ARGS__); sp::traceOpenGLCall("glGetVertexAttribiv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glGetVertexAttribPointerv(...) do { glGetVertexAttribPointerv(__VA_ARGS__); sp::traceOpenGLCall("glGetVertexAttribPointerv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glHint(...) do { glHint(__VA_ARGS__); sp::traceOpenGLCall("glHint", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glLineWidth(...) do { glLineWidth(__VA_ARGS__); sp::traceOpenGLCall("glLineWidth", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glLinkProgram(...) do { glLinkProgram(__VA_ARGS__); sp::traceOpenGLCall("glLinkProgram", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glPixelStorei(...) do { glPixelStorei(__VA_ARGS__); sp::traceOpenGLCall("glPixelStorei", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glPolygonOffset(...) do { glPolygonOffset(__VA_ARGS__); sp::traceOpenGLCall("glPolygonOffset", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glReadPixels(...) do { glReadPixels(__VA_ARGS__); sp::traceOpenGLCall("glReadPixels", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glReleaseShaderCompiler(...) do { glReleaseShaderCompiler(__VA_ARGS__); sp::traceOpenGLCall("glReleaseShaderCompiler", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glRenderbufferStorage(...) do { glRenderbufferStorage(__VA_ARGS__); sp::traceOpenGLCall("glRenderbufferStorage", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glSampleCoverage(...) do { glSampleCoverage(__VA_ARGS__); sp::traceOpenGLCall("glSampleCoverage", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glScissor(...) do { glScissor(__VA_ARGS__); sp::traceOpenGLCall("glScissor", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glShaderBinary(...) do { glShaderBinary(__VA_ARGS__); sp::traceOpenGLCall("glShaderBinary", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glShaderSource(...) do { glShaderSource(__VA_ARGS__); sp::traceOpenGLCall("glShaderSource", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glStencilFunc(...) do { glStencilFunc(__VA_ARGS__); sp::traceOpenGLCall("glStencilFunc", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glStencilFuncSeparate(...) do { glStencilFuncSeparate(__VA_ARGS__); sp::traceOpenGLCall("glStencilFuncSeparate", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glStencilMask(...) do { glStencilMask(__VA_ARGS__); sp::traceOpenGLCall("glStencilMask", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glStencilMaskSeparate(...) do { glStencilMaskSeparate(__VA_ARGS__); sp::traceOpenGLCall("glStencilMaskSeparate", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glStencilOp(...) do { glStencilOp(__VA_ARGS__); sp::traceOpenGLCall("glStencilOp", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glStencilOpSeparate(...) do { glStencilOpSeparate(__VA_ARGS__); sp::traceOpenGLCall("glStencilOpSeparate", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glTexImage2D(...) do { glTexImage2D(__VA_ARGS__); sp::traceOpenGLCall("glTexImage2D", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glTexParameterf(...) do { glTexParameterf(__VA_ARGS__); sp::traceOpenGLCall("glTexParameterf", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glTexParameterfv(...) do { glTexParameterfv(__VA_ARGS__); sp::traceOpenGLCall("glTexParameterfv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glTexParameteri(...) do { glTexParameteri(__VA_ARGS__); sp::traceOpenGLCall("glTexParameteri", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glTexParameteriv(...) do { glTexParameteriv(__VA_ARGS__); sp::traceOpenGLCall("glTexParameteriv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glTexSubImage2D(...) do { glTexSubImage2D(__VA_ARGS__); sp::traceOpenGLCall("glTexSubImage2D", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform1f(...) do { glUniform1f(__VA_ARGS__); sp::traceOpenGLCall("glUniform1f", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform1fv(...) do { glUniform1fv(__VA_ARGS__); sp::traceOpenGLCall("glUniform1fv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform1i(...) do { glUniform1i(__VA_ARGS__); sp::traceOpenGLCall("glUniform1i", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform1iv(...) do { glUniform1iv(__VA_ARGS__); sp::traceOpenGLCall("glUniform1iv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform2f(...) do { glUniform2f(__VA_ARGS__); sp::traceOpenGLCall("glUniform2f", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform2fv(...) do { glUniform2fv(__VA_ARGS__); sp::traceOpenGLCall("glUniform2fv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform2i(...) do { glUniform2i(__VA_ARGS__); sp::traceOpenGLCall("glUniform2i", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform2iv(...) do { glUniform2iv(__VA_ARGS__); sp::traceOpenGLCall("glUniform2iv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform3f(...) do { glUniform3f(__VA_ARGS__); sp::traceOpenGLCall("glUniform3f", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform3fv(...) do { glUniform3fv(__VA_ARGS__); sp::traceOpenGLCall("glUniform3fv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform3i(...) do { glUniform3i(__VA_ARGS__); sp::traceOpenGLCall("glUniform3i", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform3iv(...) do { glUniform3iv(__VA_ARGS__); sp::traceOpenGLCall("glUniform3iv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform4f(...) do { glUniform4f(__VA_ARGS__); sp::traceOpenGLCall("glUniform4f", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform4fv(...) do { glUniform4fv(__VA_ARGS__); sp::traceOpenGLCall("glUniform4fv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform4i(...) do { glUniform4i(__VA_ARGS__); sp::traceOpenGLCall("glUniform4i", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniform4iv(...) do { glUniform4iv(__VA_ARGS__); sp::traceOpenGLCall("glUniform4iv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniformMatrix2fv(...) do { glUniformMatrix2fv(__VA_ARGS__); sp::traceOpenGLCall("glUniformMatrix2fv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniformMatrix3fv(...) do { glUniformMatrix3fv(__VA_ARGS__); sp::traceOpenGLCall("glUniformMatrix3fv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUniformMatrix4fv(...) do { glUniformMatrix4fv(__VA_ARGS__); sp::traceOpenGLCall("glUniformMatrix4fv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glUseProgram(...) do { glUseProgram(__VA_ARGS__); sp::traceOpenGLCall("glUseProgram", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glValidateProgram(...) do { glValidateProgram(__VA_ARGS__); sp::traceOpenGLCall("glValidateProgram", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glVertexAttrib1f(...) do { glVertexAttrib1f(__VA_ARGS__); sp::traceOpenGLCall("glVertexAttrib1f", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glVertexAttrib1fv(...) do { glVertexAttrib1fv(__VA_ARGS__); sp::traceOpenGLCall("glVertexAttrib1fv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glVertexAttrib2f(...) do { glVertexAttrib2f(__VA_ARGS__); sp::traceOpenGLCall("glVertexAttrib2f", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glVertexAttrib2fv(...) do { glVertexAttrib2fv(__VA_ARGS__); sp::traceOpenGLCall("glVertexAttrib2fv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glVertexAttrib3f(...) do { glVertexAttrib3f(__VA_ARGS__); sp::traceOpenGLCall("glVertexAttrib3f", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glVertexAttrib3fv(...) do { glVertexAttrib3fv(__VA_ARGS__); sp::traceOpenGLCall("glVertexAttrib3fv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glVertexAttrib4f(...) do { glVertexAttrib4f(__VA_ARGS__); sp::traceOpenGLCall("glVertexAttrib4f", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glVertexAttrib4fv(...) do { glVertexAttrib4fv(__VA_ARGS__); sp::traceOpenGLCall("glVertexAttrib4fv", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glVertexAttribPointer(...) do { glVertexAttribPointer(__VA_ARGS__); sp::traceOpenGLCall("glVertexAttribPointer", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)
#define glViewport(...) do { glViewport(__VA_ARGS__); sp::traceOpenGLCall("glViewport", __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::traceOpenGLCallParams(__VA_ARGS__)); } while(0)

#endif//SP_ENABLE_OPENGL_TRACING

#endif//SP_GRAPHICS_OPENGL_H
