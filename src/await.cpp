#include "include/await.h"

#include <coroutine>
#include <future>
#include <iostream>
using namespace std;

AWait::AWait() {}

AWait::~AWait() {}

Task AWait::test() {
    int value = co_await MyAwaitable{};

    std::cout << value << '\n';
}