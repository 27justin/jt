// Copyright (c) 2025 Justin Andreas Lacoste <me@justin.cx>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// SPDX-License-Identifier: MIT

// A very simple event bus using jt::spmc<>
// Messages are duplicated and sent to each consumer.
// Each consumer holds it's own queue, thus the implementation is reasonably fast.
// Though it can lead to high memory usage when the producer dispatches more
// messages than can be handled by the consumers.
//
// Each message is duplicated N times, where N is the number of active consumers

#include <chrono>
#include <format>
#include <iostream>
#include <jt.hpp>
#include <thread>

int
main(int argc, char *argv[]) {
    jt::spmc<std::string> bus{};

    auto       &producer = bus.tx();
    std::thread main([&producer] {
        // Send a `Heartbeat N' every second to the bus
        for (int i = 0;; ++i) {
            producer(std::format("Heartbeat #{}", i));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });

    for (auto i = 0; i < 4; ++i) {
        auto        rx = bus.rx();
        std::thread th([rx = jt::spmc<std::string>::consumer(std::move(rx)), i] {
            std::string value;
            for (;;) {
                value = rx.wait();
                std::cout << std::format("Consumer #{}: \"{}\"\n", i, value);
            }
        });
        th.detach();
    }
    main.join();
    return 0;
}
