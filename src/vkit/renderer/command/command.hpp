#pragma once

namespace vkit::renderer::command {

class Command {
 public:
  virtual ~Command() = default;

  virtual void record(vk::CommandBuffer cb) const = 0;
};

class Lambda : public Command {
 public:
  using RecordFunction = std::function<void(vk::CommandBuffer)>;

  explicit Lambda(RecordFunction func) : func_{std::move(func)} {}

  void record(vk::CommandBuffer cb) const override {
    if (func_) {
      func_(cb);
    }
  }

 private:
  RecordFunction func_;
};

};  // namespace vkit::renderer::command
