// C++26 Reflection Example
// This example demonstrates the reflection features proposed for C++26 (P2996)
// Note: Requires a compiler with C++26 experimental reflection support

#include <experimental/meta>
#include <format>
#include <iostream>
#include <string>
#include <string_view>

// Example struct to reflect upon
struct Point {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;

  std::string label;
};

// Example with methods
struct WellData {
  std::string name;
  double depth;
  double pressure;
  bool isActive;

  double getPressureGradient() const { return pressure / depth; }
};

// Template function to print all members of a struct using reflection
template <typename T> void printMembers(const T &obj) {
  std::cout << std::format("Members of {}:\n", std::meta::name_of(^T));

  // Iterate over all non-static data members
  [:std::meta::nonstatic_data_members_of(^T):] >> [&]<auto... members> {
    auto printMember = [&]<auto member>() {
      // Get member name and value
      constexpr auto memberName = std::meta::name_of(member);
      auto &value = obj.[:member:];

      // Print based on type
      using MemberType = typename[:std::meta::type_of(member):];

      if constexpr (std::is_same_v<MemberType, std::string>) {
        std::cout << std::format("  {}: \"{}\"\n", memberName, value);
      } else if constexpr (std::is_same_v<MemberType, double>) {
        std::cout << std::format("  {}: {:.2f}\n", memberName, value);
      } else if constexpr (std::is_same_v<MemberType, bool>) {
        std::cout << std::format("  {}: {}\n", memberName,
                                 value ? "true" : "false");
      } else {
        std::cout << std::format("  {}: <value>\n", memberName);
      }
    };

    (printMember.template operator()<members>(), ...);
  };

  std::cout << "\n";
}

// Template function to count members
template <typename T> consteval std::size_t memberCount() {
  return std::meta::nonstatic_data_members_of(^T).size();
}

// Template function to check if type has a specific member
template <typename T, std::string_view Name> consteval bool hasMember() {
  bool found = false;
  [:std::meta::nonstatic_data_members_of(^T):] >> [&]<auto... members> {
    ((std::meta::name_of(members) == Name ? found = true : false), ...);
  };
  return found;
}

// Example of reflection-based serialization
template <typename T> std::string toJson(const T &obj) {
  std::string result = "{\n";

  [:std::meta::nonstatic_data_members_of(^T):] >> [&]<auto... members> {
    bool first = true;
    auto serializeMember = [&]<auto member>() {
      if (!first)
        result += ",\n";
      first = false;

      constexpr auto memberName = std::meta::name_of(member);
      auto &value = obj.[:member:];

      using MemberType = typename[:std::meta::type_of(member):];

      if constexpr (std::is_same_v<MemberType, std::string>) {
        result += std::format("  \"{}\": \"{}\"", memberName, value);
      } else if constexpr (std::is_arithmetic_v<MemberType>) {
        result += std::format("  \"{}\": {}", memberName, value);
      } else if constexpr (std::is_same_v<MemberType, bool>) {
        result +=
            std::format("  \"{}\": {}", memberName, value ? "true" : "false");
      }
    };

    (serializeMember.template operator()<members>(), ...);
  };

  result += "\n}";
  return result;
}

int main() {
  std::cout << "=== C++26 Reflection Example ===\n\n";

  // Example 1: Basic member printing
  Point p{10.5, 20.3, 30.7, "Surface Point"};
  std::cout << "Example 1: Print struct members\n";
  printMembers(p);

  // Example 2: Member count
  std::cout << "Example 2: Member count\n";
  std::cout << std::format("Point has {} members\n", memberCount<Point>());
  std::cout << std::format("WellData has {} members\n\n",
                           memberCount<WellData>());

  // Example 3: Check for specific members
  std::cout << "Example 3: Check for specific members\n";
  std::cout << std::format("Point has 'x' member: {}\n",
                           hasMember<Point, "x">() ? "true" : "false");
  std::cout << std::format("Point has 'name' member: {}\n",
                           hasMember<Point, "name">() ? "true" : "false");
  std::cout << std::format("WellData has 'depth' member: {}\n\n",
                           hasMember<WellData, "depth">() ? "true" : "false");

  // Example 4: JSON serialization using reflection
  WellData well{"Well-A", 3500.0, 5200.0, true};
  std::cout << "Example 4: Reflection-based JSON serialization\n";
  std::cout << "WellData:\n";
  printMembers(well);
  std::cout << "JSON representation:\n";
  std::cout << toJson(well) << "\n\n";

  // Example 5: List all member names at compile-time
  std::cout << "Example 5: List all member names of Point\n";
  [:std::meta::nonstatic_data_members_of(^Point):] >> []<auto... members> {
    ((std::cout << std::format("  - {}\n", std::meta::name_of(members))), ...);
  };

  return 0;
}
