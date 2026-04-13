/**
 * @file math_utils.hpp
 * @brief 解析器向けの基本統計ユーティリティを定義する.
 */

#pragma once

#include <algorithm>
#include <vector>

namespace transient_tracker::analyze {

/**
 * @brief ベクトルの中央値を返す.
 * @tparam T 値型
 * @param values 入力値列
 * @return 中央値
 */
template <typename T>
T ComputeMedian(std::vector<T>* values) {
    if (values == nullptr || values->empty()) {
        return static_cast<T>(0);
    }

    const std::size_t middle = values->size() / 2;
    std::nth_element(values->begin(), values->begin() + middle, values->end());
    T median = (*values)[middle];
    if (values->size() % 2 == 0) {
        std::nth_element(values->begin(), values->begin() + middle - 1, values->end());
        median = static_cast<T>(0.5) * (median + (*values)[middle - 1]);
    }
    return median;
}

}  // namespace transient_tracker::analyze
