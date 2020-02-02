#include <iostream>
#include <vector>
#include <functional>
#include <utility>
#include <deque>
#include <variant>

template <typename Message, typename Result>
struct Request {
    const Message m;
    const std::function<void(Result &&)> f;
};

template<typename ...Messages>
class BusImpl {

    template<typename Message>
    using CallbackT = std::function<void(const Message &)>;
    template<typename Message>
    using Subscribers = std::vector<CallbackT<Message>>;

    std::tuple<Subscribers<Messages>...> subscribers;

    template<typename Head, typename ...Tail>
    void subscribe(Head& head, Tail& ...tail) {
        subscribe(head);
        subscribe(tail...);
    }

    template<typename Head>
    void subscribe(Head& head) {
        head.subscribeSelf(*this);
    }

    using MessagesVariant = std::variant<Messages...>;
    std::deque<MessagesVariant> queue;

    template<typename...>
    struct typelist {};

    template <typename MessageT>
    bool callIfMatches(const MessagesVariant &message) {
        if (std::holds_alternative<MessageT>(message)) {
            auto &currentSubscribers = std::get<Subscribers<MessageT>>(subscribers);
            for (auto callback : currentSubscribers) {
                callback(std::get<MessageT>(message));
            }
            return true;
        }
        return false;
    }

    template<typename Head, typename ...Tail>
    void callCallbacks(const MessagesVariant &message, typelist<Head, Tail...>) {
        if (!callIfMatches<Head>(message)) {
            callCallbacks(message, typelist<Tail...>());
        }
    }

    template<typename Head>
    void callCallbacks(const MessagesVariant &message, typelist<Head>) {
        if (!callIfMatches<Head>(message)) {
            std::cout << "Message not present" << std::endl;
            throw std::runtime_error("AA");
        }
    }

public:

    BusImpl() = default;

    template<typename ...Subscribers>
    constexpr BusImpl(Subscribers& ...subscribers) {
        subscribe(subscribers...);
    }

    template<typename ...Subscribers>
    constexpr void subscribeAll(Subscribers& ...subscribers) {
        subscribe(subscribers...);
    }

    template<typename Message>
    void subscribe(CallbackT<Message> &&function) {
        std::get<Subscribers<Message>>(subscribers).emplace_back(std::move(function));
    }

    template<typename Message>
    void sendMessage(Message &&msg) {
        queue.emplace_back(std::forward<Message>(msg));
    }

    bool processMessage() {
        if (queue.empty()) {
            return false;
        }
        auto message = std::move(queue.front());
        queue.pop_front();
        callCallbacks(message, typelist<Messages...>());

        return true;
    }
};
