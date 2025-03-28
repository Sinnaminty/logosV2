#pragma once

#include <string>
#include <unordered_map>
namespace Oot {

class OOTItemHints {
 private:
  static std::unordered_map<std::string, std::string> m_itemToLocation;

 public:
  static void init(const std::string& jsonData);
  static bool isReady();
  static std::string getItemLocation(const std::string& itemName);
};

}  // namespace Oot
