#pragma once

#include <filesystem>
#include <optional>
#include <span>

namespace vkit::platform {

class FileDialogContext {
 public:
  FileDialogContext();
  ~FileDialogContext();

  FileDialogContext(const FileDialogContext&) = delete;
  FileDialogContext& operator=(const FileDialogContext&) = delete;
};

struct FileFilter {
  const char* name;
  const char* spec;
};

auto openFileDialog(std::span<const FileFilter> filters = {},
                    const std::filesystem::path& default_path = {})
    -> std::optional<std::filesystem::path>;

};  // namespace vkit::platform
