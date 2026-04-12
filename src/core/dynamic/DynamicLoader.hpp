#pragma once
#include <cstddef>
#include <dlfcn.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

template<std::size_t N>
struct fixed_string {
    char value[N]{};

    // ReSharper disable once CppNonExplicitConvertingConstructor
    constexpr fixed_string(const char (&str)[N]) {
        for (std::size_t i = 0; i < N; ++i)
            value[i] = str[i];
    }
};

template<std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

template<typename Func, fixed_string Name>
struct Member {
    using function_type = Func;

    Func func = nullptr;

    void load(void *lib) {
        ::dlerror();
        this->func = reinterpret_cast<Func>(dlsym(lib, Name.value));
        if (const char *error = ::dlerror(); error != nullptr)
            throw std::runtime_error("Failed to load symbol '" + std::string(Name.value) + "': " + error);
    }

    template<typename MemberType, typename... Args>
    std::invoke_result_t<typename MemberType::function_type, Args...>
    call(Args &&... args) {
        return static_cast<MemberType &>(*this)(std::forward<Args>(args)...);
    }
};


template<typename... Members>
class DynamicLoader : public Members... {
    void *lib = nullptr;

public:
    explicit DynamicLoader(const std::string &path) {
        lib = ::dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
        if (lib == nullptr)
            throw std::runtime_error("Failed to open shared library '" + path + "': " + ::dlerror());
        (Members::load(lib), ...);
    }

    ~DynamicLoader() {
        // Intentionally not calling dlclose(lib) here.
        // Unloading complex graphical libraries (SFML, SDL, ncurses) on Linux
        // often causes segmentation faults due to dangling signal handlers,
        // thread-local storage corruption, or improperly destroyed OpenGL contexts.
        // Keeping them in memory is the standard, safest workaround.
    }

    DynamicLoader(const DynamicLoader &) = delete;
    DynamicLoader &operator=(const DynamicLoader &) = delete;
    DynamicLoader(DynamicLoader &&) = delete;
    DynamicLoader &operator=(DynamicLoader &&) = delete;

    template<typename MemberType, typename... Args>
    decltype(auto) call(Args &&... args) {
        return static_cast<MemberType &>(*this).func(std::forward<Args>(args)...);
    }
};
