#include "dynamicLibrary.h"

#include "logging.h"

#include <string_view>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
using handle_t = HMODULE;
static constexpr std::string_view native_extension{ ".dll" };
#else // assume posix
#include <dlfcn.h>
using handle_t = void*;
#ifdef __APPLE__
static constexpr std::string_view native_extension{ ".dylib" };
#else
static constexpr std::string_view native_extension{ ".so" };
#endif
#endif

struct DynamicLibrary::Impl final
{
    explicit Impl(handle_t handle)
        :handle{ handle }
    {}

    Impl(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl& operator=(Impl&&) = delete;
    ~Impl();

    handle_t handle{};
};

DynamicLibrary::Impl::~Impl()
{
    if (!handle)
        return;

#if defined(_WIN32)
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif
}

[[nodiscard]]
std::unique_ptr<DynamicLibrary> DynamicLibrary::open(const std::filesystem::path& filepath)
{
    std::unique_ptr<DynamicLibrary> library;
#if defined(_WIN32)
    auto handle = LoadLibraryW(filepath.c_str());
#else
    auto handle = dlopen(filepath.c_str(), RTLD_LAZY);
#endif

    if (handle)
        library.reset(new DynamicLibrary(std::make_unique<Impl>(handle)));
    
    return library;
}

std::filesystem::path DynamicLibrary::add_native_suffix(const std::filesystem::path& basepath)
{
    auto result = basepath;
    result += native_extension;
    return result;
}

template<>
void* DynamicLibrary::getFunction<void*>(std::string_view name)
{
    if (!impl->handle)
        return nullptr;
        
#if defined(_WIN32)
    return reinterpret_cast<void*>(GetProcAddress(impl->handle, name.data()));
#else
    return dlsym(impl->handle, name.data());
#endif
}

DynamicLibrary::~DynamicLibrary() = default;

void* DynamicLibrary::nativeHandle() const
{
    return impl->handle;
}

DynamicLibrary::DynamicLibrary(std::unique_ptr<Impl>&& impl)
    :impl{ std::move(impl) }
{
}
