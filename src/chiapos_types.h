#ifndef DEPINC_CHIAPOS_COMMON_TYPES_H
#define DEPINC_CHIAPOS_COMMON_TYPES_H

#include <optional>
#include <cstdint>
#include <vector>

using CAmount = int64_t;
static const CAmount COIN = 100000000;

namespace chiapos {

using Bytes = std::vector<uint8_t>;

template <typename T>
using optional = std::optional<T>;

}  // namespace chiapos

#endif
