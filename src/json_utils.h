#pragma once

#include <nlohmann/json.hpp>

#include "common_header.h"

class JsonUtils {
 public:
  static void populatePriceLevels(Levels &levels,
                                  const nlohmann::json &jsonLevels,
                                  bool withSequence = false);
};
