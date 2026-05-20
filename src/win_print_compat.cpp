// GCC's std::print on Windows calls these two functions to write Unicode
// text to the Windows console handle.  When cross-compiling the Windows
// target from Linux the libstdc++ library doesn't carry their implementations,
// so the linker fails.  Returning false from __open_terminal makes
// std::vprint_unicode fall back to a plain fwrite, which is perfectly fine
// for our use case.

#if defined(_WIN32)
#include <cstdio>
#include <span>

namespace std {
bool __open_terminal(std::FILE*) noexcept { return false; }
void __write_to_terminal(void*, std::span<char>) noexcept {}
}  // namespace std

#endif  // _WIN32
