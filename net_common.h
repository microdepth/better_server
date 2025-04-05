// Created by psdab on 5/2/2024.

#ifndef BETTER_SERVER_NET_COMMON_H
#define BETTER_SERVER_NET_COMMON_H
#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <optional>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <type_traits>

#ifdef _WIN32 // specifies which version of windows to use, because apparently every version of windows handles networking slightly differently
#define _WIN32_WINNT 0x0A00
#endif

#define ASIO_STANDALONE // asio comes from the boost library, so this is telling the program to use asio on its own

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

#endif