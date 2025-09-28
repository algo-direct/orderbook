#pragma once

#include <chrono>
#include <limits>

const double PRICE_COMPARISION_TOLERANCE = 1e-12;
const double SIZE_COMPARISION_TOLERANCE = 1e-12;

using PriceType = double;
using SizeType = double;
using SequenceType = std::size_t;
using TimePoint = std::chrono::milliseconds;

bool priceCompareEqual(const PriceType &lhs, const PriceType &rhs);
bool sizeCompareEqual(const SizeType &lhs, const SizeType &rhs);
bool priceCompareLessThan(const SizeType &lhs, const SizeType &rhs);
bool priceCompareGreaterThan(const SizeType &lhs, const SizeType &rhs);
bool sizeCompareLessThan(const SizeType &lhs, const SizeType &rhs);

struct PriceCompareLessThan {
  bool operator()(const PriceType &lhs, const PriceType &rhs) const;
};

struct PriceCompareGreaterThan {
  bool operator()(const PriceType &lhs, const PriceType &rhs) const;
};

std::string httpGet(std::string_view host, std::string_view port,
                    std::string_view uri);
