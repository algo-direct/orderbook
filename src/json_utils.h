#pragma once

#include <nlohmann/json.hpp>

#include "common_header.h"

class OrderBook;

class JsonUtils {
 public:
  static void populatePriceLevels(Levels& levels,
                                  const nlohmann::json& jsonLevels,
                                  bool withSequence = false);

  static std::string orderBookSnapshotToJson(const OrderBook& orderBook);
};
