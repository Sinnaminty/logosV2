#include <Logos/Oot.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <unordered_map>

using json = nlohmann::json;

namespace Oot {
std::unordered_map<std::string, std::string> OOTItemHints::m_itemToLocation;

void OOTItemHints::init(const std::string& json_data) {
  std::cout << "Init Called!!\n";
  json data = json::parse(json_data);

  if (data.contains("locations")) {
    for (auto& [location, item] : data["locations"].items()) {
      if (item.is_string()) {
        m_itemToLocation[item] = location;
        std::cout << "item: " << item << "location: " << location << std::endl;
      } else if (item.is_object() && item.contains("item")) {
        std::cout << "item:[item]" << item["item"] << "location: " << location
                  << std::endl;
        m_itemToLocation[item["item"]] = location;
      }
    }
  } else {
    std::cout << "Cannot find locations!!\n";
  }
}

std::string OOTItemHints::getItemLocation(const std::string& itemName) {
  auto it = m_itemToLocation.find(itemName);
  if (it != m_itemToLocation.end()) {
    return it->second;
  }
  return "Item not found in the hint data.";
}

bool OOTItemHints::isReady() {
  return (!m_itemToLocation.empty());
}

}  // namespace Oot
