#pragma once

namespace vkit::graphics {

class Command {
 public:
  virtual ~Command() = default;

  virtual void record(vk::CommandBuffer cb) const = 0;
};

class LambdaCommand : public Command {
 public:
  using RecordFunction = std::function<void(vk::CommandBuffer)>;

  explicit LambdaCommand(RecordFunction func) : func_{std::move(func)} {}

  void record(vk::CommandBuffer cb) const override {
    if (func_) {
      func_(cb);
    }
  }

 private:
  RecordFunction func_;
};

};  // namespace vkit::graphics
