#ifndef SP_GRAPHICS_OPENGL_H
#define SP_GRAPHICS_OPENGL_H

#include <glad/glad.h>

namespace sp {
	void initOpenGL();
}//namespace sp


#ifdef DEBUG
#define SP_ENABLE_OPENGL_TRACING
#endif


#ifdef SP_ENABLE_OPENGL_TRACING
#include <type_traits>
#include <functional>

#include <SDL_assert.h>
#include <stringImproved.h>

namespace sp {
	namespace details {
		void traceOpenGLCall(const char* function_name, const char* source_file, const char* source_function, int source_line_number, const string& parameters);
		static inline string traceOpenGLCallParams() { return ""; }
		template<typename T> std::enable_if_t<std::is_integral_v<T>, string> traceOpenGLCallParam(T n) { return string(static_cast<int>(n)); }
		template<typename T> std::enable_if_t<std::is_floating_point_v<T>, string> traceOpenGLCallParam(T n) { return string(static_cast<float>(n)); }
		template<typename T> std::enable_if_t<std::is_pointer_v<T>, string> traceOpenGLCallParam(T) { return "[ptr]"; }
		template<typename T> std::enable_if_t<std::is_null_pointer_v<T>, string> traceOpenGLCallParam(T) { return "[null]"; }
		template<typename A1> string traceOpenGLCallParams(A1&& a) { return traceOpenGLCallParam(std::forward<A1>(a)); }
		template<typename A1, typename... ARGS> string traceOpenGLCallParams(A1&& a, ARGS&&... args) { return traceOpenGLCallParam(std::forward<A1>(a)) + ", " + traceOpenGLCallParams(std::forward<ARGS>(args)...); }
	}

	template<typename F, typename... ARGS>
	auto glWrapper(const char* function_name, const char* source_file, const char* source_function, int source_line_number, const string& parameters, F&& function, ARGS&&... args)
	{
		static_assert(std::is_pointer_v<std::remove_reference_t<F>>, "Expects function to be a C function pointer!");
		SDL_assert_always(function != nullptr);
		if constexpr (!std::is_void_v<std::invoke_result_t<F, ARGS...>>)
		{
			auto result = std::invoke(std::forward<F>(function), std::forward<ARGS>(args)...);
			details::traceOpenGLCall(function_name, source_file, source_function, source_line_number, parameters);
			return result;
		}
		else
		{
			std::invoke(std::forward<F>(function), std::forward<ARGS>(args)...);
			details::traceOpenGLCall(function_name, source_file, source_function, source_line_number, parameters);
		}
	}

}//namespace sp

#ifdef _MSC_VER
#define SP_PRETTY_FUNCTION __FUNCTION__
#else
#define SP_PRETTY_FUNCTION __PRETTY_FUNCTION__
#endif

#define SP_TRACE_OPENGL_CALL(name, function, ...) sp::glWrapper(name, __FILE__, SP_PRETTY_FUNCTION, __LINE__, sp::details::traceOpenGLCallParams(__VA_ARGS__), function,##__VA_ARGS__)

#include "graphics/glDebug.inl"
#endif // SP_ENABLE_OPENGL_TRACING
#endif//SP_GRAPHICS_OPENGL_H
