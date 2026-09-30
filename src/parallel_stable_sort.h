/*
    Copyright © 2026 Michael Shields

    Use of this source code is governed by a BSD-style license that can be found
    in the LICENSE.txt file.
*/

#pragma once

#include <algorithm>
#include <tbb/parallel_invoke.h>

template <typename Iterator, typename Compare>
void parallel_stable_sort(Iterator begin, Iterator end, Compare compare) {
    if (end - begin <= 2048) {
        std::stable_sort(begin, end, compare);
        return;
    }

    Iterator middle = begin + (end - begin) / 2;
    tbb::parallel_invoke(
        [=] { parallel_stable_sort(begin, middle, compare); },
        [=] { parallel_stable_sort(middle, end, compare); });
    std::inplace_merge(begin, middle, end, compare);
}
