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

    /// Calls subscribed callbacks if the message type matches.
    template <typename Message>
    bool callIfMatches(const MessagesVariant &message) {
        if (std::holds_alternative<Message>(message)) {
            auto &currentSubscribers = std::get<Subscribers<Message>>(subscribers);
            for (auto callback : currentSubscribers) {
                callback(std::get<Message>(message));
            }
            return true;
        }
        return false;
    }

    /// Variadic functions to iterate through all the message types and find and call particular subscribers.
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
    /// Allows subscription of arbitrary number of subscribers.
    /// It is expected that subscribers implement the function subscribeSelf() which is called by
    /// the function subscribe().
    template<typename ...Subscribers>
    constexpr BusImpl(Subscribers& ...subscribers) {
        subscribe(subscribers...);
    }
    /// The same functionality as variadic constructor above.
    template<typename ...Subscribers>
    constexpr void subscribeAll(Subscribers& ...subscribers) {
        subscribe(subscribers...);
    }
    /// Subscribes one subscriber for given message.
    template<typename Message>
    void subscribe(Subscriber<Message> &&subscriber) {
        std::get<Subscribers<Message>>(subscribers).emplace_back(std::move(subscriber));
    }
    /// Enqueues the message to the message queue.
    template<typename Message>
    void sendMessage(Message &&msg) {
        queue.emplace_back(std::forward<Message>(msg));
    }
    /// Extracts one message from the message queue and executes given subscribers.
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

#ifdef REQUESTOR_DEMO
/// Helper message to provide one-shot-request functionality.
/// When the message is received and processed, the response should called immediately.
/// This type is primarily used by the BaseRequestor (and derived) class.
template <typename Message, typename Result>
struct Request {
    const Message message;
    const std::function<void(Result &&)> response;
};

/// Base for requests of various types. The object of this class subscribes itself to the message that forms a request
/// and when the message is received, it processes it (calls the action defined in derived class) and immediately
/// calls the response callback.
template <typename Bus, typename Message, typename Result>
class BaseRequestor {
public:
    void subscribeSelf(Bus &bus) {
        bus. template subscribe<Request<Message, Result>>(std::bind(&BaseRequestor::onRequest, &*this, std::placeholders::_1));
    }

private:
    virtual Result action(const Message &) = 0;

    void onRequest(const Request<Message, Result> &request) {
        request.response(action(request.message));
    }
};
#endif
