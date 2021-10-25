# Generate tracer
# GLAD defines all gl* function like so:
# #define gl* glad_gl*
# To support our own debug facilities, provide a way to masquerade the calls with SP tracer.
# This generates a glDebug.inl file, with the following snippet for each gl* define:
# #undef gl*
# #define gl* SP_TRACE_OPENGL_CALL(/* snip */)

# Get all the gl* function from glad's header.
message(STATUS "GLAD header: ${glad_h}")
message(STATUS "GlDebug.inl output: ${glDebug_inl}")
if(NOT EXISTS "${glad_h}")
    message(FATAL_ERROR "GLAD header not found")
endif()
file(STRINGS "${glad_h}" glad_gl_functions REGEX "^#define gl.+ glad_.+$")

# Replace the functions with our own wrapper.
set(gl_functions)
foreach(entry IN LISTS glad_gl_functions)
	string(REGEX REPLACE "#define (gl.+) (glad_.+)" "#undef \\1\n#define \\1(...) SP_TRACE_OPENGL_CALL(\"\\1\", \\2,##__VA_ARGS__)" traced ${entry})
	list(APPEND gl_functions "${traced}")
endforeach()
string(REPLACE ";" "\n" gl_functions "${gl_functions}")

# Generate inline file.
configure_file(cmake/glDebug.inl.in "${glDebug_inl}")