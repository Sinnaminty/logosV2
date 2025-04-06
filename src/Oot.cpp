#include <Logos/Oot.h>
#include <algorithm>
#include <dpp/nlohmann/json.hpp>
#include <iostream>
#include <unordered_map>

using json = nlohmann::json;

namespace Oot {
std::unordered_map<std::string, std::vector<std::string>>
    OOTItemHints::m_itemToLocation;

void OOTItemHints::init(const std::string& json_data) {
  m_itemToLocation.clear();

  json data = json::parse(json_data);

  if (data.contains("locations")) {
    for (auto& [location, item] : data["locations"].items()) {
      if (item.is_string()) {
        m_itemToLocation[item].push_back(location);
      } else if (item.is_object() && item.contains("item")) {
        m_itemToLocation[item["item"]].push_back(location);
      }
    }
  } else {
    std::cout << "Cannot find locations!!\n";
  }
}

std::vector<std::string> OOTItemHints::getItemLocations(
    const std::string& itemName) {
  auto it = m_itemToLocation.find(itemName);
  if (it != m_itemToLocation.end()) {
    return it->second;
  }
  return {"Item not found in the hint data."};
}

std::string OOTItemHints::getItemFromLocation(const std::string& locationName) {
  std::vector<std::string> keys;
  for (const auto& pair : m_itemToLocation) {
    for (const auto& location : pair.second) {
      if (location == locationName) {
        return pair.first;
      }
    }
  }
  return "Location not found.";
}

}  // namespace Oot
