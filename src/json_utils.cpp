
#include "json_utils.h"

#include "utils.h"

void JsonUtils::populatePriceLevels(Levels& levels,
                                    const nlohmann::json& jsonLevels,
                                    bool withSequence) {
  levels.reserve(std::size(jsonLevels));
  for (const auto& jsonLevel : jsonLevels) {
    const auto& bidSizeAndSequence = jsonLevel.get<std::vector<std::string>>();
    if (withSequence) {
      if (size(bidSizeAndSequence) != 3) {
        auto error = std::format(
            "Expect 3 elements in price level json array, but found {}, input "
            "json array '{}'",
            std::size(bidSizeAndSequence), jsonLevel.dump());
        throw std::runtime_error(error);
      }
    } else {
      if (size(bidSizeAndSequence) != 2) {
        auto error = std::format(
            "Expect 2 elements in price level json array, but found {}, input "
            "json array '{}'",
            std::size(bidSizeAndSequence), jsonLevel.dump());
        throw std::runtime_error(error);
      }
    }
    if (bidSizeAndSequence[0].empty()) {
      auto error = std::format("Bid/Ask string is empty, input json array '{}'",
                               jsonLevel.dump());
      throw std::runtime_error(error);
    }
    if (bidSizeAndSequence[1].empty()) {
      auto error = std::format("Size string is empty, input json array '{}'",
                               jsonLevel.dump());
      throw std::runtime_error(error);
    }
    if (bidSizeAndSequence[2].empty()) {
      auto error = std::format(
          "Sequence string is empty, input json array '{}'", jsonLevel.dump());
      throw std::runtime_error(error);
    }
    levels.push_back({.price = std::stod(bidSizeAndSequence[0]),
                      .size = std::stod(bidSizeAndSequence[1])});

    if (withSequence) {
      levels.back().sequence = std::stoull(bidSizeAndSequence[2]);
    }

    if (sizeCompareLessThan(levels.back().size, 0)) {
      auto error = std::format("Size {} is less than 0", bidSizeAndSequence[0]);
      throw std::runtime_error(error);
    }
    if (priceCompareLessThan(levels.back().price, 0)) {
      auto error =
          std::format("Price {} is less than 0", bidSizeAndSequence[0]);
      throw std::runtime_error(error);
    }
  }
}
