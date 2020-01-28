#include <iostream>
#include <vector>
#include <functional>
#include <utility>
#include <deque>
#include <variant>

template<typename ...Messages>
class BusImpl {

    template<typename Message>
    using CallbacksT = std::vector<std::function<void(Message &&)>>;

    template<typename Message>
    struct Subscriber {
        CallbacksT<Message> callbacks;
    };

    std::tuple<Subscriber<Messages>...> subscribers;

    template<typename Head, typename ...Tail>
    void subscribe(Head head, Tail ...tail) {
        subscribe(head);
        subscribe(tail...);
    }

    template<typename Head>
    void subscribe(Head head) {
        head.subscribeSelf(*this);
    }

    using MessagesVariant = std::variant<Messages...>;
    std::deque<MessagesVariant> queue;

    template<typename...>
    struct typelist {
    };

    template<typename Head, typename ...Tail>
    void getCallbacks(MessagesVariant &&messages, typelist<Head, Tail...>) {
        if (std::holds_alternative<Head>(messages)) {
            for (auto callback : std::get<Subscriber<Head>>(subscribers).callbacks) {
                callback(std::move(std::get<Head>(messages)));
            }
        } else {
            getCallbacks(std::forward<MessagesVariant>(messages), typelist<Tail...>());
        }
    }

    template<typename Head>
    void getCallbacks(MessagesVariant &&messages, typelist<Head>) {
        if (std::holds_alternative<Head>(messages)) {
            for (auto callback : std::get<Subscriber<Head>>(subscribers).callbacks) {
                callback(std::move(std::get<Head>(messages)));
            }
        } else {
            std::cout << "Message not present" << std::endl;
            throw std::runtime_error("AA");
        }
    }

public:
    template<typename ...Subscribers>
    constexpr BusImpl(Subscribers ...subscribers) {
        subscribe(subscribers...);
    }

    template<typename Message>
    void subscribe(std::function<void(Message&&)> &&function) {
        std::get<Subscriber<Message>>(subscribers).callbacks.emplace_back(function);
    }

    template<typename Message>
    void sendMessage(Message &&msg) {
        queue.emplace_back(std::forward<Message>(msg));
    }

    bool processMessage() {
        if (queue.empty()) {
            return false;
        }
        auto msg = queue.front();
        queue.pop_front();
        getCallbacks(std::move(msg), typelist<Messages...>());

        return true;
    }
};
