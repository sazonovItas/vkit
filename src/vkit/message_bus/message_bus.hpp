#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

namespace vkit::message_bus {

enum class DispatchPolicy { kSyncOnly, kQueueOnly, kBoth };

template <typename MessagType, DispatchPolicy Policy>
class MessageBus;

template <typename MessageType>
class Subscription {
 public:
  Subscription() = default;

  struct Detail {
    std::function<void(MessageType&)> callback;
    void invoke(MessageType& message) { callback(message); }
  };

 private:
  template <typename M, DispatchPolicy P>
  friend class MessageBus;

  explicit Subscription(std::shared_ptr<Detail> detail) : detail_{std::move(detail)} {}

  std::shared_ptr<Detail> detail_;
};

template <typename MessageType, DispatchPolicy Policy = DispatchPolicy::kBoth>
class MessageBus {
 public:
  template <typename Callback>
  auto subscribe(Callback&& callback) -> Subscription<MessageType> {
    auto detail = std::make_shared<typename Subscription<MessageType>::Detail>(
        typename Subscription<MessageType>::Detail{
            std::function<void(MessageType&)>{std::forward<Callback>(callback)}});
    std::lock_guard<std::mutex> lock{receiversMutex_};
    receivers_.push_back(detail);
    return Subscription<MessageType>{std::move(detail)};
  }

  void send_message(MessageType message)
    requires(Policy == DispatchPolicy::kSyncOnly || Policy == DispatchPolicy::kBoth)
  {
    std::lock_guard<std::mutex> lock{receiversMutex_};
    bool has_expired = false;
    for (auto& weak_detail : receivers_) {
      auto detail = weak_detail.lock();
      if (!detail) {
        has_expired = true;
        continue;
      }
      detail->invoke(message);
    }
    if (has_expired) {
      prune_expired();
    }
  }

  void queue_message(MessageType message)
    requires(Policy == DispatchPolicy::kQueueOnly || Policy == DispatchPolicy::kBoth)
  {
    std::lock_guard<std::mutex> lock{queuedMessagesMutex_};
    queuedMessages_.push(message);
  }

  void update()
    requires(Policy == DispatchPolicy::kQueueOnly || Policy == DispatchPolicy::kBoth)
  {
    std::lock_guard<std::mutex> lock_1{receiversMutex_};
    {
      std::lock_guard<std::mutex> lock_2{messagesMutex_};
      bool has_expired = false;
      while (!messages_.empty()) {
        for (auto& weak_detail : receivers_) {
          auto detail = weak_detail.lock();
          if (!detail) {
            has_expired = true;
            continue;
          }
          detail->invoke(messages_.front());
        }
        messages_.pop();
      }
      if (has_expired) {
        prune_expired();
      }
      {
        std::lock_guard<std::mutex> lock_3{queuedMessagesMutex_};
        messages_ = std::move(queuedMessages_);
      }
    }
  }

 private:
  void prune_expired() {
    receivers_.erase(std::remove_if(receivers_.begin(), receivers_.end(),
                                    [](const auto& w) { return w.expired(); }),
                     receivers_.end());
  }

  std::mutex receiversMutex_;
  std::vector<std::weak_ptr<typename Subscription<MessageType>::Detail>> receivers_;

  std::mutex messagesMutex_;
  std::queue<MessageType> messages_;

  std::mutex queuedMessagesMutex_;
  std::queue<MessageType> queuedMessages_;
};

}  // namespace vkit::message_bus
