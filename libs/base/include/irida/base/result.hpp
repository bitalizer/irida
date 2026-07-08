// SPDX-License-Identifier: BUSL-1.1
// Copyright (c) 2026 Bitalizer.
#pragma once
#include <cassert>
#include <string>
#include <utility>

namespace irida::base {

template <typename T> class Result {
  public:
    static Result ok(T value) {
        return Result(std::move(value));
    }
    static Result err(std::string message) {
        return Result(std::move(message), true);
    }

    bool has_value() const {
        return has_value_;
    }
    const T& value() const {
        assert(has_value_);
        return value_;
    }
    const std::string& error() const {
        assert(!has_value_);
        return error_;
    }

  private:
    explicit Result(T value) : value_(std::move(value)), has_value_(true) {}
    Result(std::string message, bool) : error_(std::move(message)), has_value_(false) {}

    T value_{};
    std::string error_{};
    bool has_value_{false};
};

} // namespace irida::base
