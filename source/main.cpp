#include <iostream>
#include <chrono>
#include <cassert>

#include "Bus.hpp"
#include "BusRuntime.hpp"

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
struct RunProcess { const std::string command; };
using RunProcessRequest = Request<RunProcess, std::string>;
//------------------------------------------------------------------------------------------------------
// Bus definition
//------------------------------------------------------------------------------------------------------
using Bus = BusImpl<MsgA, MsgB, MsgC, MsgD, MsgE, RunProcessRequest>;
//------------------------------------------------------------------------------------------------------
// Communicating components
// 1) General listeners: Listener1, Listener2, Listener3
// 2) ProcessRunner and ProcessRunnerClient: demonstration of a request
//------------------------------------------------------------------------------------------------------
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
        bus.subscribe<MsgA>(std::bind(&Listener1::onMsgA, &*this, std::placeholders::_1));
        bus.subscribe<MsgB>(std::bind(&Listener1::onMsgB, &*this, std::placeholders::_1));
        bus.subscribe<MsgC>([](const MsgC &) { print("Listener1 received MsgC"); });
        bus.subscribe<MsgD>(std::bind(&Listener1::onMsgD, &*this, std::placeholders::_1));
        bus.subscribe<MsgE>([](const MsgE &) { print("Listener1 received MsgE"); });
    }

    void subscribeSelf(BusRuntime &bus) {
        bus.subscribe<MsgA>(std::bind(&Listener1::onMsgA, &*this, std::placeholders::_1));
        bus.subscribe<MsgB>(std::bind(&Listener1::onMsgB, &*this, std::placeholders::_1));
        bus.subscribe<MsgC>([](const MsgC &) { print("Listener1 received MsgC"); });
        bus.subscribe<MsgD>(std::bind(&Listener1::onMsgD, &*this, std::placeholders::_1));
        bus.subscribe<MsgE>([](const MsgE &) { print("Listener1 received MsgE"); });
    }
};

struct Listener2 {
    CTORS(Listener2)
    void onMsgB(const MsgB &) {
        print("Listener2 received MsgB");
    }

    void onMsgC(const MsgC &) {
        print("Listener2 received MsgC");
    }

    void subscribeSelf(Bus &bus) {
        bus.subscribe<MsgB>(std::bind(&Listener2::onMsgB, &*this, std::placeholders::_1));
        bus.subscribe<MsgC>(std::bind(&Listener2::onMsgC, &*this, std::placeholders::_1));
    }

    void subscribeSelf(BusRuntime&bus) {
        bus.subscribe<MsgB>(std::bind(&Listener2::onMsgB, &*this, std::placeholders::_1));
        bus.subscribe<MsgC>(std::bind(&Listener2::onMsgC, &*this, std::placeholders::_1));
    }
};

struct Listener3 {
    CTORS(Listener3)
    void onMsgB(const MsgB &) {
        print("Listener2 received MsgB");
    }

    void onMsgC(const MsgC &) {
        print("Listener2 received MsgC");
    }

    void subscribeSelf(Bus &bus) {
        bus.subscribe<MsgB>(std::bind(&Listener3::onMsgB, &*this, std::placeholders::_1));
        bus.subscribe<MsgC>(std::bind(&Listener3::onMsgC, &*this, std::placeholders::_1));
        bus.subscribe<MsgD>([](const MsgD &) { print("Listener3 received MsgD"); });
        bus.subscribe<MsgE>([](const MsgE &) { print("Listener3 received MsgE"); });
    }

    void subscribeSelf(BusRuntime&bus) {
        bus.subscribe<MsgB>(std::bind(&Listener3::onMsgB, &*this, std::placeholders::_1));
        bus.subscribe<MsgC>(std::bind(&Listener3::onMsgC, &*this, std::placeholders::_1));
        bus.subscribe<MsgD>([](const MsgD &) { print("Listener3 received MsgD"); });
        bus.subscribe<MsgE>([](const MsgE &) { print("Listener3 received MsgE"); });
    }
};

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
//------------------------------------------------------------------------------------------------------
// Time measurement
//------------------------------------------------------------------------------------------------------
template <typename F>
void measureTime(F &&f, const int iterations = 5000) {

    int64_t sum = 0;
    for (int i = 0; i < iterations; ++i) {
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
void processing(BusT &&bus, const int iterations = 500) {

    for (int i = 0; i < iterations; ++i) {
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
void measure(const int iterations = 5000) {
    Listener1 l1;
    Listener2 l2;
    Listener3 l3;

    Bus b1(l1, l2, l3);
    BusRuntime b2(l1, l2, l3);

    std::cout << "Compile-time bus: ";
    measureTime([&]() {
        processing(b1, iterations < 10 ? 1 : iterations/10);
    }, iterations);
    print("-----------------------------------");
    std::cout << "Run-time bus: ";
    measureTime([&]() {
        processing(b2, iterations < 10 ? 1 : iterations/10);
    }, iterations);
}

void demo() {
    Bus bus;
    Listener1 l1;
    ProcessRunner p;
    ProcessRunnerClient pc{bus};

    bus.subscribeAll(l1, p, pc);

    bus.sendMessage(MsgA());

    while (bus.processMessage());
}
//------------------------------------------------------------------------------------------------------
int main() {

    demo();
    measure();

    return 0;
}
