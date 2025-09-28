#pragma once

#include <chrono>
#include <limits>

#include "common_header.h"

bool priceCompareEqual(const PriceType &lhs, const PriceType &rhs);
bool sizeCompareEqual(const SizeType &lhs, const SizeType &rhs);
bool priceCompareLessThan(const SizeType &lhs, const SizeType &rhs);
bool priceCompareGreaterThan(const SizeType &lhs, const SizeType &rhs);
bool sizeCompareLessThan(const SizeType &lhs, const SizeType &rhs);

std::string httpGet(std::string_view host, std::string_view port,
                    std::string_view uri);
