#include "vkit/platform/file_dialog.hpp"

#include <nfd.hpp>
#include <vector>

namespace vkit::platform {

FileDialogContext::FileDialogContext() { NFD::Init(); }

FileDialogContext::~FileDialogContext() { NFD::Quit(); }

auto openFileDialog(std::span<const FileFilter> filters,
                    const std::filesystem::path& default_path)
    -> std::optional<std::filesystem::path> {
  std::vector<nfdnfilteritem_t> nfd_filters;
  nfd_filters.reserve(filters.size());
  for (const auto& f : filters) {
    nfd_filters.push_back({.name = f.name, .spec = f.spec});
  }

  const nfdnchar_t* default_path_str =
      default_path.empty() ? nullptr : default_path.c_str();

  nfdnchar_t* out_path = nullptr;
  nfdresult_t result = NFD::OpenDialog(
      out_path, nfd_filters.data(),
      static_cast<nfdfiltersize_t>(nfd_filters.size()), default_path_str);

  if (result == NFD_OKAY && out_path) {
    std::filesystem::path path(out_path);
    NFD::FreePath(out_path);
    return path;
  }
  return std::nullopt;
}

auto saveFileDialog(std::span<const FileFilter> filters,
                    const std::filesystem::path& default_path,
                    const char* default_name)
    -> std::optional<std::filesystem::path> {
  std::vector<nfdnfilteritem_t> nfd_filters;
  nfd_filters.reserve(filters.size());
  for (const auto& f : filters) {
    nfd_filters.push_back({.name = f.name, .spec = f.spec});
  }

  const nfdnchar_t* default_path_str =
      default_path.empty() ? nullptr : default_path.c_str();

  nfdnchar_t* out_path = nullptr;
  nfdresult_t result =
      NFD::SaveDialog(out_path, nfd_filters.data(),
                      static_cast<nfdfiltersize_t>(nfd_filters.size()),
                      default_path_str, default_name);

  if (result == NFD_OKAY && out_path) {
    std::filesystem::path path(out_path);
    NFD::FreePath(out_path);
    return path;
  }
  return std::nullopt;
}

};  // namespace vkit::platform
