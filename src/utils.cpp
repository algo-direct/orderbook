
#include "utils.h"

#include <cmath>

bool doubleCompareEqual(const SizeType& lhs, const SizeType& rhs,
                        double tolerance) {
  if (std::abs(lhs - rhs) < tolerance) {
    return true;
  }
  return false;
}

bool doubleCompareLessThan(const SizeType& lhs, const SizeType& rhs,
                           double tolerance) {
  if (doubleCompareEqual(lhs, rhs, tolerance)) {
    return false;
  }
  return lhs < rhs;
}

bool priceCompareEqual(const PriceType& lhs, const PriceType& rhs) {
  if (std::abs(lhs - rhs) < PRICE_COMPARISION_TOLERANCE) {
    return true;
  }
  return false;
}

bool sizeCompareEqual(const SizeType& lhs, const SizeType& rhs) {
  if (std::abs(lhs - rhs) < SIZE_COMPARISION_TOLERANCE) {
    return true;
  }
  return false;
}

bool priceCompareLessThan(const SizeType& lhs, const SizeType& rhs) {
  if (priceCompareEqual(lhs, rhs)) {
    return false;
  }
  return lhs < rhs;
}

bool priceCompareGreaterThan(const SizeType& lhs, const SizeType& rhs) {
  if (priceCompareEqual(lhs, rhs)) {
    return false;
  }
  return lhs > rhs;
}

bool sizeCompareLessThan(const SizeType& lhs, const SizeType& rhs) {
  if (sizeCompareEqual(lhs, rhs)) {
    return false;
  }
  return lhs < rhs;
}

bool PriceCompareLessThan::operator()(const PriceType& lhs,
                                      const PriceType& rhs) const {
  return priceCompareLessThan(lhs, rhs);
}

bool PriceCompareGreaterThan::operator()(const PriceType& lhs,
                                         const PriceType& rhs) const {
  return priceCompareGreaterThan(lhs, rhs);
}

std::string httpGet(std::string_view host, std::string_view port,
                    std::string_view uri);