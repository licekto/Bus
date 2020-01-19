#include <iostream>
#include <vector>
#include <functional>
#include <utility>
#include <deque>
#include <variant>

template<typename ...Messages>
class BusImpl {

    template <typename Message>
    using CallbacksT = std::vector<std::function<void(Message &&)>>;

    template <typename Message>
    struct Subscriber {
        CallbacksT<Message> callbacks;
    };

    std::tuple<Subscriber<Messages>...> subscribers;

    template <typename Head, typename ...Tail>
    void subscribe(Head head, Tail ...tail) {
        subscribe(head);
        subscribe(tail...);
    }

    template <typename Head>
    void subscribe(Head head) {
        head.subscribeSelf(*this);
    }

    using MessagesVariant = std::variant<Messages...>;
    std::deque<MessagesVariant> queue;

    template<typename...>
    struct typelist{};

    template <typename Head, typename ...Tail>
    void getCallbacks(MessagesVariant &&messages, typelist<Head, Tail...>) {
        if (std::holds_alternative<Head>(messages)) {
            for (auto callback : std::get<Subscriber<Head>>(subscribers).callbacks) {
                callback(std::move(std::get<Head>(messages)));
            }
        } else {
            getCallbacks(std::forward<MessagesVariant>(messages), typelist<Tail...>());
        }
    }

    template <typename Head>
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
    template <typename ...Subscribers>
    BusImpl(Subscribers ...subscribers) {
        subscribe(subscribers...);
    }

    template <typename Message, typename Function>
    void subscribe(Function &&function) {
        std::get<Subscriber<Message>>(subscribers).callbacks.emplace_back(function);
    }

    template <typename Message>
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

struct MsgA {};
struct MsgB {};
struct MsgC {};

using Bus = BusImpl<MsgA, MsgB, MsgC>;

struct Listener1 {
    void onMsgA(const MsgA &) {
        std::cout << "Listener1 received MsgA" << std::endl;
    }

    void onMsgB(const MsgB &) {
        std::cout << "Listener1 received MsgB" << std::endl;
    }

    void subscribeSelf(Bus &bus) {
        bus.subscribe<MsgA>(std::bind(&Listener1::onMsgA, *this, std::placeholders::_1));
        bus.subscribe<MsgB>(std::bind(&Listener1::onMsgB, *this, std::placeholders::_1));
        bus.subscribe<MsgC>([](const MsgC &) { std::cout << "Listener1 received MsgC" << std::endl; });
    }
};

struct Listener2 {
    void onMsgB(const MsgB &) {
        std::cout << "Listener2 received MsgB" << std::endl;
    }

    void onMsgC(const MsgC &) {
        std::cout << "Listener2 received MsgC" << std::endl;
    }

    void subscribeSelf(Bus &bus) {
        bus.subscribe<MsgB>(std::bind(&Listener2::onMsgB, *this, std::placeholders::_1));
        bus.subscribe<MsgC>(std::bind(&Listener2::onMsgC, *this, std::placeholders::_1));
    }
};

int main() {
    Listener1 l1;
    Listener2 l2;

    Bus bus(l1, l2);

    bus.sendMessage(MsgA());
    bus.sendMessage(MsgB());
    bus.sendMessage(MsgA());
    bus.sendMessage(MsgC());
    bus.sendMessage(MsgC());
    bus.sendMessage(MsgB());
    bus.sendMessage(MsgB());

    while (bus.processMessage());
    return 0;
}