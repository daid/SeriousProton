#ifndef SP_DYNAMIC_LIBRARY_H
#define SP_DYNAMIC_LIBRARY_H

#ifdef __ANDROID__
#error Unsupported
#endif

#include <memory>
#include <filesystem>

/*! Dynamic library (dll, so, dylib) wrapper. */
class DynamicLibrary final
{
public:
    /*! Load a dynamic library.
    * 
    The system does not try to auto-deduce extension.

    \param filepath path to the library to load.
    \return a pointer to the library (if it loaded successfully).
    */
    [[nodiscard]]
    static std::unique_ptr<DynamicLibrary> open(const std::filesystem::path& filepath);

    /*! Adds the native platform suffix to the basepath.
    
    \param basepath extension-less library path.
    \return filepath with native extension.
    */
    static std::filesystem::path add_native_suffix(const std::filesystem::path& basepath);
    
    /*! Retrieve a function pointer into the library.
    
    \param name Name of the function to load.
    \return function pointer. nullptr if not found.

    \remarks
        The template is provided as a convenience:
        Libraries essentially returning void pointer,
        no guarantee can be made about type correctness.
    */
    template<typename Function>
    Function getFunction(std::string_view name);

    /*! Get the native handle. Platform-specific. */
    void* nativeHandle() const;

    ~DynamicLibrary();
private:
    struct Impl;

    explicit DynamicLibrary(std::unique_ptr<Impl>&&);

    std::unique_ptr<Impl> impl;
};


template<>
void* DynamicLibrary::getFunction<void*>(std::string_view name);
extern template void* DynamicLibrary::getFunction<void*>(std::string_view);

template<typename Function>
Function DynamicLibrary::getFunction(std::string_view name)
{
    return reinterpret_cast<Function>(getFunction<void*>(name));
}

#endif // SP_DYNAMIC_LIBRARY_H
