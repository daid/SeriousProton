#include "dynamicLibrary.h"

#include <SDL_loadso.h>

#include "logging.h"

#include <string_view>

#if defined(_WIN32)
static constexpr std::string_view native_extension{ ".dll" };
#elif defined(__APPLE__) 
static constexpr std::string_view native_extension{ ".dylib" };
#else // assume posix
static constexpr std::string_view native_extension{ ".so" };
#endif

struct DynamicLibrary::Impl final
{
    explicit Impl(void* handle)
        :handle{ handle }
    {}

    Impl(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl& operator=(Impl&&) = delete;
    ~Impl();

    void* handle{};
};

DynamicLibrary::Impl::~Impl()
{
    if (handle)
        SDL_UnloadObject(handle);
}

[[nodiscard]]
std::unique_ptr<DynamicLibrary> DynamicLibrary::open(const std::filesystem::path& filepath)
{
    std::unique_ptr<DynamicLibrary> library;
    auto handle = SDL_LoadObject(filepath.string().c_str());

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
    
    return SDL_LoadFunction(impl->handle, name.data());
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
