#include <vector>
#include <functional>
#include <variant>
#include <deque>

/**
 * Simple compile-time Message Bus implementation.
 * @tparam Messages Messages that are processed by the bus. All the messages have to be known beforehand.
 *
 * The bus contains tuple of subscribers to the messages. Each tuple element is vector of subscribers and
 * each subscriber is represented by a callback.
 * When a message arrives, all the callbacks for the message are executed and the message is passed as parameter.
 *
 * All the subscribers are expected to implement the subscribeSelf() function which is called by bus when
 * multiple subscribers are to be subscribed at once.
 * Subscriber may also subscribe itself by simply calling the subscribe() method.
 *
 * Message are (so far) processed sequentially by the queue inside of the bus.
 */
template<typename ...Messages>
class BusImpl {
    /// General type for subscriber, its parameter is Message that is to be received.
    template<typename Message>
    using Subscriber = std::function<void(const Message &)>;
    /// Vector of all the subscribers for particular message.
    template<typename Message>
    using Subscribers = std::vector<Subscriber<Message>>;
    /// Type-erased container for all the message types, used by the queue
    using MessagesVariant = std::variant<Messages...>;
    /// Tuple of subscribers: [ {messageA: { s1, s2, s3}}, {messageB: { s2 }}, {messageC: {s1, s3}} ... ]
    std::tuple<Subscribers<Messages>...> subscribers;
    /// Queue of sent messages
    std::deque<MessagesVariant> queue;

    /// Variadic functions to iterate through all the subscribers so that they can subscribe themselves.
    template<typename Head, typename ...Tail>
    void subscribe(Head& head, Tail& ...tail) {
        subscribe(head);
        subscribe(tail...);
    }
    template<typename Head>
    void subscribe(Head& head) {
        head.subscribeSelf(*this);
    }

    /// Helper type that allows iteration through all the messages to find appropriate subscribers.
    /// The type itself is a dummy, that is not used, but serves only as variadic iteration variable.
    template<typename...>
    struct typelist {};

    /// Calls 
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
    void subscribe(Subscriber<Message> &&function) {
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

template <typename Message, typename Result>
struct Request {
    const Message message;
    const std::function<void(Result &&)> response;
};

template <typename Bus, typename Message, typename Result>
class BaseRequestor {
public:
    using RequestT = Request<Message, Result>;
    void subscribeSelf(Bus &bus) {
        bus. template subscribe<RequestT>(std::bind(&BaseRequestor::onRequest, &*this, std::placeholders::_1));
    }

private:
    virtual Result action(const Message &) = 0;

    void onRequest(const RequestT &request) {
        request.response(action(request.message));
    }
};