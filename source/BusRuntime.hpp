#include <iostream>
#include <vector>
#include <functional>
#include <utility>
#include <deque>
#include <any>
#include <typeindex>

namespace {
template <typename T>
std::type_index getType() {
    return std::type_index(typeid(T));
}
}

class BusRuntime {

    template<typename Message>
    using CallbacksT = std::vector<std::function<void(const Message &)>>;

    template<typename Message>
    struct Subscriber {
        CallbacksT<Message> callbacks;
    };

    std::unordered_map<std::type_index, std::any> subscribers;

    std::deque<std::function<void()>> queue;

    template<typename Head, typename ...Tail>
    void subscribe(Head head, Tail ...tail) {
        subscribe(head);
        subscribe(tail...);
    }

    template<typename Head>
    void subscribe(Head head) {
        head.subscribeSelf(*this);
    }

public:
    template<typename ...Subscribers>
    constexpr BusRuntime(Subscribers&& ...subscribers) {
        subscribe(std::forward<Subscribers>(subscribers)...);
    }

    template<typename Message>
    void subscribe(std::function<void(const Message&)> &&function) {
        //subscribers.emplace({getType<Message>(), function});
        decltype(std::begin(subscribers)) it = std::end(subscribers);
        if (auto iit = subscribers.find(getType<Message>()); iit == std::end(subscribers)) {
            it = subscribers.emplace(getType<Message>(), std::make_any<Subscriber<Message>>()).first;
        }
        if (it == std::end(subscribers)) {
            it = subscribers.find(getType<Message>());
        }
        std::any_cast<Subscriber<Message>>(&it->second)->callbacks.push_back(function);
    }

    template<typename Message>
    void sendMessage(Message &&msg) {
        queue.emplace_back([this, msg]() {
            auto it = subscribers.find(getType<Message>());
            if (it == std::end(subscribers)) {
                std::cout << "Message not found" << std::endl;
                return;
            }
            auto subscriber = std::any_cast<Subscriber<Message>>(&it->second);
            auto callbacks = subscriber->callbacks;

            for (auto callback : callbacks) {
                callback(msg);
            }
        });
    }

    bool processMessage() {
        if (queue.empty()) {
            return false;
        }
        auto msg = queue.front();
        queue.pop_front();
        msg();

        return true;
    }
};
