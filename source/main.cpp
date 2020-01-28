#include <iostream>
#include <chrono>

#include "Bus.hpp"
#include "BusRuntime.hpp"

struct MsgA {};
struct MsgB {};
struct MsgC {};
struct MsgD {};
struct MsgE {};

using Bus = BusImpl<MsgA, MsgB, MsgC, MsgD, MsgE>;

struct Listener1 {
    void onMsgA(const MsgA &) {
        //std::cout << "Listener1 received MsgA" << std::endl;
    }

    void onMsgB(const MsgB &) {
        //std::cout << "Listener1 received MsgB" << std::endl;
    }

    void onMsgD(const MsgD &) {
        //std::cout << "Listener1 received MsgD" << std::endl;
    }

    void subscribeSelf(Bus &bus) {
        bus.subscribe<MsgA>(std::bind(&Listener1::onMsgA, *this, std::placeholders::_1));
        bus.subscribe<MsgB>(std::bind(&Listener1::onMsgB, *this, std::placeholders::_1));
        bus.subscribe<MsgC>([](const MsgC &) { /*std::cout << "Listener1 received MsgC" << std::endl;*/ });
        bus.subscribe<MsgD>(std::bind(&Listener1::onMsgD, *this, std::placeholders::_1));
        bus.subscribe<MsgE>([](const MsgE &) { /*std::cout << "Listener1 received MsgE" << std::endl;*/ });
    }

    void subscribeSelf(BusRuntime &bus) {
        bus.subscribe<MsgA>(std::bind(&Listener1::onMsgA, *this, std::placeholders::_1));
        bus.subscribe<MsgB>(std::bind(&Listener1::onMsgB, *this, std::placeholders::_1));
        bus.subscribe<MsgC>([](const MsgC &) { /*std::cout << "Listener1 received MsgC" << std::endl;*/ });
        bus.subscribe<MsgD>(std::bind(&Listener1::onMsgD, *this, std::placeholders::_1));
        bus.subscribe<MsgE>([](const MsgE &) { /*std::cout << "Listener1 received MsgE" << std::endl;*/ });
    }
};

struct Listener2 {
    void onMsgB(const MsgB &) {
        //std::cout << "Listener2 received MsgB" << std::endl;
    }

    void onMsgC(const MsgC &) {
        //std::cout << "Listener2 received MsgC" << std::endl;
    }

    void subscribeSelf(Bus &bus) {
        bus.subscribe<MsgB>(std::bind(&Listener2::onMsgB, *this, std::placeholders::_1));
        bus.subscribe<MsgC>(std::bind(&Listener2::onMsgC, *this, std::placeholders::_1));
    }

    void subscribeSelf(BusRuntime&bus) {
        bus.subscribe<MsgB>(std::bind(&Listener2::onMsgB, *this, std::placeholders::_1));
        bus.subscribe<MsgC>(std::bind(&Listener2::onMsgC, *this, std::placeholders::_1));
    }
};

struct Listener3 {
    void onMsgB(const MsgB &) {
        //std::cout << "Listener2 received MsgB" << std::endl;
    }

    void onMsgC(const MsgC &) {
        //std::cout << "Listener2 received MsgC" << std::endl;
    }

    void subscribeSelf(Bus &bus) {
        bus.subscribe<MsgB>(std::bind(&Listener3::onMsgB, *this, std::placeholders::_1));
        bus.subscribe<MsgC>(std::bind(&Listener3::onMsgC, *this, std::placeholders::_1));
        bus.subscribe<MsgD>([](const MsgD &) { /*std::cout << "Listener3 received MsgD" << std::endl;*/ });
        bus.subscribe<MsgE>([](const MsgE &) { /*std::cout << "Listener3 received MsgE" << std::endl;*/ });

    }

    void subscribeSelf(BusRuntime&bus) {
        bus.subscribe<MsgB>(std::bind(&Listener3::onMsgB, *this, std::placeholders::_1));
        bus.subscribe<MsgC>(std::bind(&Listener3::onMsgC, *this, std::placeholders::_1));
        bus.subscribe<MsgD>([](const MsgD &) { /*std::cout << "Listener3 received MsgD" << std::endl;*/ });
        bus.subscribe<MsgE>([](const MsgE &) { /*std::cout << "Listener3 received MsgE" << std::endl;*/ });
    }
};

template <typename F>
void measureTime(F &&f) {
    auto now = std::chrono::system_clock::now();

    f();

    auto then = std::chrono::system_clock::now();
    auto duration = then - now;
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(duration).count() << "us\n";
}

int main() {
    Listener1 l1;
    Listener2 l2;
    Listener2 l3;

    measureTime([&]() {
        Bus bus(l1, l2, l3);

        bus.sendMessage(MsgA());
        bus.sendMessage(MsgB());
        bus.sendMessage(MsgA());
        bus.sendMessage(MsgC());
        bus.sendMessage(MsgC());
        bus.sendMessage(MsgB());
        bus.sendMessage(MsgB());

        while (bus.processMessage());
    });

    //std::cout << "----------------------------------\n";
    measureTime([&]() {
        BusRuntime bus(l1, l2, l3);

        bus.sendMessage(MsgA());

        bus.sendMessage(MsgB());
        bus.sendMessage(MsgA());
        bus.sendMessage(MsgC());
        bus.sendMessage(MsgC());
        bus.sendMessage(MsgB());
        bus.sendMessage(MsgB());

        while (bus.processMessage());
    });

    return 0;
}