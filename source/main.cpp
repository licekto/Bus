#include <iostream>
#include <chrono>
#include <cassert>
#include <functional>

#define COMPILET_BUS
#define REQUESTOR_DEMO
#define RUNT_BUS

#ifdef COMPILET_BUS
#include "Bus.hpp"
#endif

#ifdef RUNT_BUS
#include "BusRuntime.hpp"
#endif

// Helpers
//------------------------------------------------------------------------------------------------------
#define CTORS(Name) \
    Name() {std::cout << "Default " << #Name << std::endl; }\
    Name(const Name&) { std::cout << "Copy " << #Name << std::endl; } \
    Name(Name&&) { std::cout << "Move " << #Name << std::endl; } \
    Name& operator=(const Name&) { std::cout << "operator& " << #Name << std::endl; return *this; } \
    Name& operator=(Name&&) { std::cout << "operator&& " << #Name << std::endl; return *this; }

void print(std::string msg) {
    const bool printMe = false;
    if (printMe) {
        std::cout << msg << std::endl;
    }
}
//------------------------------------------------------------------------------------------------------
// Messages definition
//------------------------------------------------------------------------------------------------------
struct MsgA { /*CTORS(MsgA)*/ std::string test = "abcd"; };
struct MsgB { /*CTORS(MsgB)*/ };
struct MsgC { /*CTORS(MsgC)*/ };
struct MsgD { /*CTORS(MsgD)*/ };
struct MsgE { /*CTORS(MsgE)*/ };
#ifdef COMPILET_BUS
struct RunProcess { const std::string command; };
#ifdef REQUESTOR_DEMO
using RunProcessRequest = Request<RunProcess, std::string>;
#endif
//------------------------------------------------------------------------------------------------------
// Bus definition
//------------------------------------------------------------------------------------------------------
using Bus = BusImpl<MsgA, MsgB, MsgC, MsgD, MsgE
#ifdef REQUESTOR_DEMO
                  , RunProcessRequest
#endif
>;
#endif
//------------------------------------------------------------------------------------------------------
// Communicating components
// 1) General listeners: Listener1, Listener2, Listener3
// 2) ProcessRunner and ProcessRunnerClient: demonstration of a request
//------------------------------------------------------------------------------------------------------
template <typename Bus>
struct Listener1 {
    CTORS(Listener1)
    void onMsgA(const MsgA &m) {
        assert(m.test[1] == 'b');
        print("Listener1 received MsgA");
    }

    void onMsgB(const MsgB &) {
        print("Listener1 received MsgB");
    }

    void onMsgD(const MsgD &) {
        print("Listener1 received MsgD");
    }

    void subscribeSelf(Bus &bus) {
        bus.template subscribe<MsgA>(std::bind(&Listener1::onMsgA, &*this, std::placeholders::_1));
        bus.template subscribe<MsgB>(std::bind(&Listener1::onMsgB, &*this, std::placeholders::_1));
        bus.template subscribe<MsgC>([](const MsgC &) { print("Listener1 received MsgC"); });
        bus.template subscribe<MsgD>(std::bind(&Listener1::onMsgD, &*this, std::placeholders::_1));
        bus.template subscribe<MsgE>([](const MsgE &) { print("Listener1 received MsgE"); });
    }
};

template <typename Bus>
struct Listener2 {
    CTORS(Listener2)
    void onMsgB(const MsgB &) {
        print("Listener2 received MsgB");
    }

    void onMsgC(const MsgC &) {
        print("Listener2 received MsgC");
    }

    void subscribeSelf(Bus &bus) {
        bus.template subscribe<MsgB>(std::bind(&Listener2::onMsgB, &*this, std::placeholders::_1));
        bus.template subscribe<MsgC>(std::bind(&Listener2::onMsgC, &*this, std::placeholders::_1));
    }
};

template <typename Bus>
struct Listener3 {
    CTORS(Listener3)
    void onMsgB(const MsgB &) {
        print("Listener2 received MsgB");
    }

    void onMsgC(const MsgC &) {
        print("Listener2 received MsgC");
    }

    void subscribeSelf(Bus &bus) {
        bus.template subscribe<MsgB>(std::bind(&Listener3::onMsgB, &*this, std::placeholders::_1));
        bus.template subscribe<MsgC>(std::bind(&Listener3::onMsgC, &*this, std::placeholders::_1));
        bus.template subscribe<MsgD>([](const MsgD &) { print("Listener3 received MsgD"); });
        bus.template subscribe<MsgE>([](const MsgE &) { print("Listener3 received MsgE"); });
    }
};

#ifdef COMPILET_BUS
#ifdef REQUESTOR_DEMO
struct ProcessRunnerClient {

    Bus &bus;

    void onMsgA(const MsgA &) {
        print("ProcessRunnerClient received MsgA, going to send request for process");
        bus.sendMessage(RunProcessRequest{RunProcess{"ls -la"}, std::bind(&ProcessRunnerClient::onProcessReady, &*this, std::placeholders::_1)});
    }

    void onProcessReady(std::string &&result) {
        print("Received result: '" + result + "'");
    }

    void subscribeSelf(Bus &bus) {
        bus.subscribe<MsgA>(std::bind(&ProcessRunnerClient::onMsgA, &*this, std::placeholders::_1));
    }
};

class ProcessRunner : public BaseRequestor<Bus, RunProcess, std::string> {
    std::string action(const RunProcess &message) override {
        std::cout << message.command << std::endl;
        return "-rwxr-xr-x 1 tomas tomas 110K  2. úno 22.56 bus";
    }
};
#endif
#endif
//------------------------------------------------------------------------------------------------------
// Time measurement
//------------------------------------------------------------------------------------------------------
template <typename F>
void measureTime(F &&f, const uint32_t iterations = 5000) {

    int64_t sum = 0;
    for (uint32_t i = 0; i < iterations; ++i) {
        auto now = std::chrono::system_clock::now();
        f();
        auto then = std::chrono::system_clock::now();
        auto duration = then - now;
        sum += std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }
    sum /= iterations;
    std::cout << sum << "us\n";
}
//------------------------------------------------------------------------------------------------------
// Some random message processing
//------------------------------------------------------------------------------------------------------
template <typename BusT>
void processing(BusT &&bus, const uint32_t iterations = 500) {

    for (uint32_t i = 0; i < iterations; ++i) {
        bus.sendMessage(MsgA());
        bus.sendMessage(MsgB());
        bus.sendMessage(MsgA());
        bus.sendMessage(MsgC());
        bus.sendMessage(MsgC());
        bus.sendMessage(MsgD());
        bus.sendMessage(MsgA());
        bus.sendMessage(MsgD());
        bus.sendMessage(MsgD());
        bus.sendMessage(MsgE());
        bus.sendMessage(MsgA());
        bus.sendMessage(MsgC());
        bus.sendMessage(MsgD());
        bus.sendMessage(MsgE());
        bus.sendMessage(MsgD());
        bus.sendMessage(MsgC());
        bus.sendMessage(MsgA());
    }

    while (bus.processMessage());
}
//------------------------------------------------------------------------------------------------------
// Functions building and running buses and launching processing
//------------------------------------------------------------------------------------------------------
template <typename BusT>
void runMeasurement(const uint32_t iterations) {
    Listener1<BusT> l1;
    Listener2<BusT> l2;
    Listener3<BusT> l3;

    BusT bus(l1, l2, l3);
    std::cout << "Compile-time bus: ";
    measureTime([&]() {
        processing(bus, iterations < 10 ? 1 : iterations/10);
    }, iterations);
}

void measureTime(const uint32_t iterations = 5000) {

#ifdef COMPILET_BUS
    runMeasurement<Bus>(iterations);
#endif
    print("-----------------------------------");
#ifdef RUNT_BUS
    runMeasurement<BusRuntime>(iterations);
#endif
}

#ifdef COMPILET_BUS
#ifdef REQUESTOR_DEMO
void demo() {
    Bus bus;
    Listener1<Bus> l1;
    ProcessRunner p;
    ProcessRunnerClient pc{bus};

    bus.subscribeAll(l1, p, pc);

    bus.sendMessage(MsgA());

    while (bus.processMessage());
}
#endif
#endif
template <typename BusT>
void measureSizeImpl() {
    Listener1<BusT> l11;
    Listener2<BusT> l12;
    Listener3<BusT> l13;
    Listener1<BusT> l21;
    Listener2<BusT> l32;
    Listener3<BusT> l33;
    Listener1<BusT> l41;
    Listener2<BusT> l42;
    Listener3<BusT> l43;
    Listener1<BusT> l51;
    Listener2<BusT> l52;
    Listener3<BusT> l53;
    Listener1<BusT> l61;
    Listener2<BusT> l62;
    Listener3<BusT> l63;
    Listener1<BusT> l71;
    Listener2<BusT> l72;
    Listener3<BusT> l73;

    BusT bus(l11, l12, l13, l21, l32, l33, l41, l42,l43, l51, l52, l53, l61, l62, l63, l71, l72, l73);
}

void measureSize() {
#ifdef COMPILET_BUS
    measureSizeImpl<Bus>();
#endif
    print("-----------------------------------");
#ifdef RUNT_BUS
    measureSizeImpl<BusRuntime>();
#endif
}
//------------------------------------------------------------------------------------------------------
int main() {
#ifdef COMPILET_BUS
#ifdef REQUESTOR_DEMO
    demo();
#endif
#endif
    measureTime();
    //measureSize();

    return 0;
}
