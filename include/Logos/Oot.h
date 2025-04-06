#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace Oot {

class OOTItemHints {
 public:
  static std::unordered_map<std::string, std::vector<std::string>>
      m_itemToLocation;
  static void init(const std::string& jsonData);
  static std::vector<std::string> getItemLocations(const std::string& itemName);
  static std::string getItemFromLocation(const std::string& locationName);
};

}  // namespace Oot
