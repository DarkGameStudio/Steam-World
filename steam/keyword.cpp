// cpp_gamedev_keywords.cpp — categorised reference of C++ keywords for game dev
// Build: g++ cpp_gamedev_keywords.cpp -o keywords -std=c++20 -O2
//        cl /EHsc /std:c++20 cpp_gamedev_keywords.cpp

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <array>
#include <sstream>

// ---------------------------------------------------------------- model
struct Kw {
    const char* name;      // keyword or identifier
    const char* since;     // C++ version it appeared in
    const char* note;      // game-dev relevance
};

struct Section {
    const char* title;
    const char* blurb;
    std::vector<Kw> items;
};

// ---------------------------------------------------------------- helpers
static void PrintRule(char c = '-', int n = 78) {
    for (int i = 0; i < n; ++i) std::cout << c;
    std::cout << "\n";
}

static void PrintSection(const Section& s) {
    std::cout << "\n";
    PrintRule('=');
    std::cout << "  " << s.title << "\n";
    PrintRule('=');
    if (s.blurb && *s.blurb) {
        std::cout << "  " << s.blurb << "\n\n";
    }
    std::cout << "  " << std::left
              << std::setw(22) << "KEYWORD"
              << std::setw(10) << "SINCE"
              << "GAME-DEV NOTE\n";
    PrintRule('-');
    for (const auto& k : s.items) {
        std::cout << "  " << std::left
                  << std::setw(22) << k.name
                  << std::setw(10) << k.since
                  << k.note << "\n";
    }
}

// ---------------------------------------------------------------- sections
static std::vector<Section> BuildReference() {
    std::vector<Section> out;

    // ============================================================ CORE TYPES
    out.push_back({
        "1. CORE TYPE KEYWORDS",
        "Fundamental types and type modifiers. Choose widths deliberately in game code — "
        "a wrong int size breaks network packets and save files.",
        {
            { "void",      "C++98",  "Function return, opaque pointers, GPU resource handles" },
            { "bool",      "C++98",  "Flags: isGrounded, isReloading, isDead" },
            { "char",      "C++98",  "Raw byte buffers, texture data, serialized packets" },
            { "signed",    "C++98",  "Explicit signedness on plain char" },
            { "unsigned",  "C++98",  "Bit flags, hashes, IDs, pool indices" },
            { "short",     "C++98",  "Rarely used; animation keys in old formats" },
            { "int",       "C++98",  "Default loop counter, score, health (32-bit)" },
            { "long",      "C++98",  "Platform-width varies — avoid on disk formats" },
            { "float",     "C++98",  "Core game math: positions, velocities, matrices" },
            { "double",    "C++98",  "Physics solvers, world coordinates, precision math" },
            { "wchar_t",   "C++98",  "Windows wide strings (UTF-16 on MSVC)" },
            { "char8_t",   "C++20",  "UTF-8 storage — modern text pipelines" },
            { "char16_t",  "C++11",  "UTF-16 code unit" },
            { "char32_t",  "C++11",  "UTF-32 code unit — full Unicode codepoint" },
            { "auto",      "C++11",  "Deduced types — iterate component pools without verbosity" },
            { "decltype",  "C++11",  "Extract expression type — template glue code" },
            { "sizeof",    "C++98",  "Compile-time size — static_assert buffer sizes" },
            { "alignof",   "C++11",  "Alignment query — SIMD, GPU buffer packing" },
            { "alignas",   "C++11",  "Force alignment for SIMD/SSE/AVX types" },
            { "nullptr",   "C++11",  "Typed null — replaces NULL in every modern engine" },
            { "true",      "C++98",  "Boolean literal" },
            { "false",     "C++98",  "Boolean literal" },
        }
    });

    // ============================================================ QUALIFIERS
    out.push_back({
        "2. DECLARATION QUALIFIERS",
        "Storage and mutability specifiers. const-correctness is the single biggest "
        "long-term win in a large codebase.",
        {
            { "const",         "C++98",  "Immutability — read-only API params, resource handles" },
            { "constexpr",     "C++11",  "Compile-time constant — baked lookup tables" },
            { "consteval",     "C++20",  "Forces compile-time evaluation (immediate fn)" },
            { "constinit",     "C++20",  "Static init guaranteed at compile time" },
            { "const_cast",    "C++98",  "Strip const — legacy C API wrappers only" },
            { "static",        "C++98",  "Translation-unit scope, class members, locals" },
            { "extern",        "C++98",  "Cross-TU linkage — C ABI, plugin entry points" },
            { "thread_local",  "C++11",  "Per-thread state — job system scratch buffers" },
            { "mutable",       "C++98",  "Mutable in const method — cached lazy data" },
            { "volatile",      "C++98",  "Not for threading — hardware registers only" },
            { "register",      "C++98",  "Deprecated C++17, removed C++20; ignored by compilers" },
            { "inline",        "C++98",  "Header-defined fns, ODR resolution" },
            { "virtual",       "C++98",  "Runtime dispatch — component behaviours, AI nodes" },
            { "override",      "C++11",  "Compiler-enforced virtual override — catches typos" },
            { "final",         "C++11",  "Seals class or override — enables devirtualisation" },
            { "explicit",      "C++98",  "Blocks implicit conversions in math types" },
            { "friend",        "C++98",  "Grant access — serialisation, operator overloads" },
            { "typedef",       "C++98",  "Legacy alias — prefer 'using' in new code" },
            { "using",         "C++11",  "Alias and scope import — modern engines favour it" },
            { "namespace",     "C++98",  "Scope — render, physics, audio, net, ai" },
            { "template",      "C++98",  "Generic code — containers, allocators, ECS storage" },
            { "typename",      "C++98",  "Dependent type disambiguation in templates" },
            { "class",         "C++98",  "Reference type; default private members" },
            { "struct",        "C++98",  "Aggregate; default public members; POD" },
            { "union",         "C++98",  "Overlapping storage — packet formats, colour packs" },
            { "enum",          "C++98",  "Named constant set — state machines, layer masks" },
            { "enum class",    "C++11",  "Scoped enum — no implicit int conversion" },
        }
    });

    // ============================================================ CONTROL FLOW
    out.push_back({
        "3. CONTROL FLOW KEYWORDS",
        "Branching and loops. Predictable branches matter more than branch count on modern CPUs.",
        {
            { "if",         "C++98",  "Branch — hot path; keep it branch-predictor friendly" },
            { "else",       "C++98",  "Alternative branch" },
            { "switch",     "C++98",  "Jump table for enum dispatch in state machines" },
            { "case",       "C++98",  "Switch label" },
            { "default",    "C++98",  "Fallback in switch; also default ctor/arg" },
            { "while",      "C++98",  "Pre-test loop" },
            { "do",         "C++98",  "Post-test loop — at-least-once iteration" },
            { "for",        "C++98",  "Counted loop — fixed-step physics ticks" },
            { "break",      "C++98",  "Exit loop or switch" },
            { "continue",   "C++98",  "Skip to next iteration" },
            { "goto",       "C++98",  "Rarely justified — cleanup paths, generated code" },
            { "return",     "C++98",  "Return from function" },
            { "co_await",   "C++20",  "Suspend coroutine — async asset loading, cutscenes" },
            { "co_return",  "C++20",  "Coroutine return — task completion" },
            { "co_yield",   "C++20",  "Yield from coroutine — generator streams" },
            { "throw",      "C++98",  "Exceptions — usually disabled in shipping builds" },
            { "try",        "C++98",  "Exception scope — often disabled with -fno-exceptions" },
            { "catch",      "C++98",  "Exception handler" },
            { "noexcept",   "C++11",  "No-throw contract — enables move optimisation" },
            { "static_assert","C++11","Compile-time check — validate struct sizes, alignments" },
        }
    });

    // ============================================================ OPERATORS
    out.push_back({
        "4. OPERATOR KEYWORDS",
        "Both spelled-out synonyms and cast operators. Games overload the math ones heavily.",
        {
            { "operator",       "C++98",  "Overload +, -, *, == on Vec3, Mat4, Quat" },
            { "new",            "C++98",  "Dynamic alloc — prefer arenas / pools in games" },
            { "delete",         "C++98",  "Free — rarely called on hot paths" },
            { "static_cast",    "C++98",  "Safe compile-time cast — preferred in game code" },
            { "dynamic_cast",   "C++98",  "Runtime cast via RTTI — often disabled for speed" },
            { "reinterpret_cast","C++98", "Bit reinterpret — packet parsing, GPU handles" },
            { "const_cast",     "C++98",  "Strip const — legacy C API interop" },
            { "typeid",         "C++98",  "Runtime type info — needs RTTI enabled" },
            { "this",           "C++98",  "Implicit self pointer" },
            { "sizeof",         "C++98",  "Object/type size in bytes" },
            { "and",            "C++98",  "Alternative for &&" },
            { "or",             "C++98",  "Alternative for ||" },
            { "not",            "C++98",  "Alternative for !" },
            { "xor",            "C++98",  "Alternative for ^" },
            { "bitand",         "C++98",  "Alternative for &" },
            { "bitor",          "C++98",  "Alternative for |" },
            { "compl",          "C++98",  "Alternative for ~" },
            { "and_eq",         "C++98",  "Alternative for &=" },
            { "or_eq",          "C++98",  "Alternative for |=" },
            { "xor_eq",         "C++98",  "Alternative for ^=" },
            { "not_eq",         "C++98",  "Alternative for !=" },
        }
    });

    // ============================================================ MODERN
    out.push_back({
        "5. MODERN C++ (11/14/17/20/23) FOR GAMES",
        "Non-keyword language features. These define how a modern engine actually looks.",
        {
            { "auto",             "C++11", "Deduced types in loops over component arrays" },
            { "range-for",        "C++11", "for (auto& e : entities) — ECS iteration" },
            { "lambda",           "C++11", "Inline closures — job dispatch, event handlers" },
            { "std::move",        "C++11", "Transfer ownership — asset loading, scene transfer" },
            { "std::forward",     "C++11", "Perfect forwarding in templated factories" },
            { "rvalue ref (&&)",  "C++11", "Move semantics — cheap transfers of large buffers" },
            { "variadic template","C++11", "Arbitrary-arg systems — event bus, logger" },
            { "std::unique_ptr",  "C++11", "Exclusive ownership — GPU buffers, scene nodes" },
            { "std::shared_ptr",  "C++11", "Shared ownership — textures, materials, atlases" },
            { "std::weak_ptr",    "C++11", "Non-owning ref — breaks cycles in scene graphs" },
            { "std::atomic",      "C++11", "Lock-free flags for job system shutdown" },
            { "std::thread",      "C++11", "Thread — usually wrapped by job system" },
            { "std::mutex",       "C++11", "Coarse lock — prefer spinlocks in hot paths" },
            { "generic lambda",   "C++14", "auto params — templated visitor patterns" },
            { "decltype(auto)",   "C++14", "Perfect return type in forwarders" },
            { "std::optional",    "C++17", "Maybe-value — asset lookup results" },
            { "std::variant",     "C++17", "Tagged union — event payloads, AI commands" },
            { "std::string_view", "C++17", "Non-owning string — zero-copy config parsing" },
            { "structured binding","C++17","auto [x, y, z] = transform — clean math code" },
            { "if constexpr",     "C++17", "Compile-time branching in templates" },
            { "inline variable",  "C++17", "Header-defined constants without ODR issues" },
            { "fold expressions", "C++17", "Variadic pack folding — accumulate, all_of" },
            { "concept",          "C++20", "Named template constraints — readable APIs" },
            { "requires",         "C++20", "Constraint clause — replaces SFINAE tricks" },
            { "module",           "C++20", "Replaces headers; faster builds on big engines" },
            { "import",           "C++20", "Module import" },
            { "three-way (<=>)",  "C++20", "Spaceship operator — auto-generated comparisons" },
            { "std::span",        "C++20", "Non-owning view of a buffer — GPU uploads" },
            { "std::bit_cast",    "C++20", "Safe type punning — no UB reinterpret_cast" },
            { "std::format",      "C++20", "Format strings — logging without printf" },
            { "std::mdspan",      "C++23", "Multi-dim array view — grid / tilemap access" },
        }
    });

    // ============================================================ ATTRIBUTES
    out.push_back({
        "6. ATTRIBUTES [[...]]",
        "Standard attributes since C++11. Compilers use these to emit better code.",
        {
            { "[[nodiscard]]",    "C++17", "Warn if return value ignored — physics queries" },
            { "[[maybe_unused]]", "C++17", "Silence unused warnings on debug-only vars" },
            { "[[likely]]",       "C++20", "Branch hint — hot path in collision test" },
            { "[[unlikely]]",     "C++20", "Branch hint — error path" },
            { "[[deprecated]]",   "C++14", "Flag old APIs during refactors" },
            { "[[fallthrough]]",  "C++17", "Silence switch fallthrough warning" },
            { "[[noreturn]]",     "C++11", "Function never returns — fatal error handler" },
            { "[[no_unique_address]]","C++20","Empty-base optimisation for stateless policies" },
        }
    });

    // ============================================================ PREPROCESSOR
    out.push_back({
        "7. PREPROCESSOR DIRECTIVES",
        "Still essential in game code — platform switches, debug toggles, API bindings.",
        {
            { "#include",   "C",     "Header inclusion — prefer forward-declares in .h" },
            { "#define",    "C",     "Macro — usually constants or API wrappers" },
            { "#undef",     "C",     "Undefine macro" },
            { "#if",        "C",     "Conditional compilation — feature flags" },
            { "#ifdef",     "C",     "Platform check — _WIN32, __ANDROID__" },
            { "#ifndef",    "C",     "Include guard, or 'not defined' check" },
            { "#elif",      "C",     "Chained conditional" },
            { "#else",      "C",     "Fallback branch" },
            { "#endif",     "C",     "End conditional" },
            { "#pragma",    "C",     "Compiler hint — once, pack, warning, optimize" },
            { "#error",     "C",     "Fail compile — unsupported platform" },
            { "#line",      "C",     "Override file/line — generated code" },
            { "#",          "C",     "Stringize macro argument" },
            { "##",         "C",     "Token paste in macros" },
            { "defined()",  "C",     "Test macro definedness in #if" },
        }
    });

    // ============================================================ COMPILER
    out.push_back({
        "8. COMPILER EXTENSIONS (non-standard, widely used in games)",
        "Every major engine uses at least a few of these. Portability cost is accepted.",
        {
            { "__forceinline",     "MSVC",   "Force inline — hottest math paths" },
            { "__declspec",        "MSVC",   "align, noinline, dllimport, dllexport" },
            { "__restrict",        "MSVC",   "Aliasing hint — vec3 math without reload" },
            { "__attribute__",     "GCC/Clang","aligned, packed, always_inline, hot, cold" },
            { "asm / __asm",       "ext",    "Inline assembly — intrinsics usually replace it" },
            { "__builtin_expect",  "GCC/Clang","Branch prediction hint (pre-C++20)" },
            { "__builtin_unreachable","GCC/Clang","Tell compiler a path can't happen" },
            { "_mm_* intrinsics",  "x86",    "SSE/AVX — vectorised math, skinning" },
            { "_Pragma",           "C++11",  "Portable pragma inside macros" },
            { "alignof / alignas", "C++11",  "Standard replacements for __declspec(align)" },
        }
    });

    // ============================================================ PLATFORM
    out.push_back({
        "9. PLATFORM MACROS & API HANDLES",
        "Not keywords, but game code is awash in them. Grouped for reference.",
        {
            { "_WIN32 / _WIN64",    "Windows", "Desktop Windows detection" },
            { "__linux__",          "Linux",   "Linux / Steam Deck" },
            { "__APPLE__",          "Apple",   "macOS / iOS" },
            { "__ANDROID__",        "Android", "Android NDK" },
            { "__EMSCRIPTEN__",     "Wasm",    "WebAssembly / browser build" },
            { "_MSC_VER",           "MSVC",    "Compiler version checks" },
            { "__clang__",          "Clang",   "Clang / Apple Clang" },
            { "__GNUC__",           "GCC",     "GCC family (also defined by Clang)" },
            { "NDEBUG",             "C",       "Release build flag — assert() becomes no-op" },
            { "_DEBUG / DEBUG",     "MSVC",    "Debug build flag" },
            { "WIN32_LEAN_AND_MEAN","Windows", "Exclude rarely-used headers" },
            { "NOMINMAX",           "Windows", "Stop min/max macros clashing with std" },
            { "APIENTRY / WINAPI",  "Windows", "Calling convention macros" },
            { "D3D12_* / ID3D12*",  "DX12",    "Direct3D 12 object and struct prefixes" },
            { "Vk*",                "Vulkan",  "Vulkan types (VkDevice, VkBuffer…)" },
            { "GL_*",               "OpenGL",  "OpenGL enum and function prefix" },
            { "SDL_*",              "SDL",     "Cross-platform window/input API" },
            { "GLFW*",              "GLFW",    "Window and context creation" },
            { "HWND / HDC",         "Win32",   "Window handle, device context" },
            { "LARGE_INTEGER",      "Win32",   "High-resolution timer value" },
        }
    });

    // ============================================================ ENGINE
    out.push_back({
        "10. ENGINE-SPECIFIC MACROS (Unreal / Unity / Godot conventions)",
        "Patterns game programmers see daily. Listed here for completeness.",
        {
            { "UCLASS / USTRUCT",   "Unreal", "Reflection metadata for the header tool" },
            { "UPROPERTY",          "Unreal", "Expose field to Blueprints / GC" },
            { "UFUNCTION",          "Unreal", "Expose method to Blueprints" },
            { "GENERATED_BODY()",   "Unreal", "Injects reflection into a class" },
            { "TArray / TMap",      "Unreal", "Engine containers" },
            { "FVector / FQuat",    "Unreal", "Engine math types" },
            { "TSharedPtr",         "Unreal", "Reference-counted smart pointer" },
            { "check / ensure",     "Unreal", "Assert macros (fatal / non-fatal)" },
            { "UE_LOG",             "Unreal", "Categorised logging macro" },
            { "GAMEPLAYATTRIBUTE",  "Unreal", "GAS attribute declaration helper" },
            { "MonoBehaviour",      "Unity",  "Base class for C# scripts (C++ via IL2CPP)" },
            { "Il2Cpp",             "Unity",  "AOT-compiled C# layer" },
            { "UNITY_INTERFACE",    "Unity",  "Native plugin entry macros" },
            { "GODOT_CLASS",        "Godot",  "Binds C++ class to the engine's scripting" },
            { "Object / Node",      "Godot",  "Core engine class hierarchy" },
            { "godot::String",      "Godot",  "Engine string type" },
        }
    });

    return out;
}

// ---------------------------------------------------------------- summary
static void PrintSummary(const std::vector<Section>& secs) {
    std::cout << "\n";
    PrintRule('=');
    std::cout << "  SUMMARY\n";
    PrintRule('=');

    int total = 0;
    for (const auto& s : secs) {
        std::cout << "  " << std::left << std::setw(52) << s.title
                  << std::setw(6) << s.items.size() << " entries\n";
        total += (int)s.items.size();
    }
    PrintRule('-');
    std::cout << "  " << std::left << std::setw(52) << "TOTAL"
              << std::setw(6) << total << "\n";
    std::cout << "\n  ISO C++ standard keywords:  ~95 (varies by standard version)\n"
              << "  Alternative tokens (and, or, ...): 11\n"
              << "  Attributes ([...]): 8+ standard, dozens compiler-specific\n"
              << "  Preprocessor directives: 15 standard\n"
              << "\n";
}

// ---------------------------------------------------------------- quick tips
static void PrintTips() {
    std::cout << "\n";
    PrintRule('=');
    std::cout << "  KEYWORD SELECTION CHEAT-SHEET FOR GAME CODE\n";
    PrintRule('=');
    std::cout <<
        "  Positions/vectors ......... float or double; use a Vec2/Vec3/Vec4 alias\n"
        "  Entity IDs ................ uint32_t or uint64_t, NOT int (signedness bugs)\n"
        "  Flags ..................... uint32_t bitmask + enum class for names\n"
        "  Resource handles .......... std::unique_ptr (owning) / raw ptr (non-owning)\n"
        "  Shared textures/materials . std::shared_ptr; break cycles with std::weak_ptr\n"
        "  Component storage ......... std::vector<T> per type, dense array\n"
        "  Hot math paths ............ constexpr + __forceinline + SIMD intrinsics\n"
        "  Per-frame scratch ......... thread_local arena, reset every tick\n"
        "  Async I/O ................. coroutines (co_await) or job system with futures\n"
        "  Config parsing ............ std::string_view — zero-copy, no allocation\n"
        "  Error handling ............ assert() in debug, __builtin_unreachable() in release\n"
        "  Serialization ............. std::bit_cast, packed structs, no vtable in packets\n"
        "  Enum dispatch ............. enum class + switch; compiler emits jump table\n"
        "  Template constraints ...... concepts (C++20) instead of SFINAE\n"
        "  Compile-time tables ....... constexpr functions + std::array\n"
        "  Logging ................... std::format / std::source_location (C++20)\n";
}

// ---------------------------------------------------------------- main
int main() {
    std::cout <<
        "==============================================================\n"
        "  C++ KEYWORDS FOR GAME DEVELOPMENT — ORGANISED REFERENCE\n"
        "  Covers ISO C++98 through C++23, plus compiler and engine\n"
        "  extensions that show up in real shipping games.\n"
        "==============================================================\n";

    auto secs = BuildReference();
    for (const auto& s : secs) PrintSection(s);
    PrintSummary(secs);
    PrintTips();

    return 0;
}