#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "riva/export.hpp"
#include "riva/status.hpp"

namespace riva {

struct JsonValue {
  enum class Type {
    kNull,
    kBool,
    kNumber,
    kString,
    kArray,
    kObject,
  };

  Type type{Type::kNull};
  bool bool_value{false};
  double number_value{0.0};
  std::string string_value;
  std::vector<JsonValue> array_value;
  std::map<std::string, JsonValue> object_value;
};

class RIVACORE_API JsonParser {
 public:
  static constexpr std::size_t kDefaultMaxInputBytes = std::size_t{64} * 1024U * 1024U;
  static constexpr std::size_t kDefaultMaxNestingDepth = 128U;

  explicit JsonParser(std::string_view input, std::size_t max_input_bytes = kDefaultMaxInputBytes,
                      std::size_t max_nesting_depth = kDefaultMaxNestingDepth);

  [[nodiscard]] bool Parse(JsonValue& value);
  [[nodiscard]] const std::string& error() const noexcept { return error_; }

 private:
  [[nodiscard]] bool ParseValue(JsonValue& value, std::size_t depth);
  [[nodiscard]] bool ParseObject(JsonValue& value, std::size_t depth);
  [[nodiscard]] bool ParseArray(JsonValue& value, std::size_t depth);
  [[nodiscard]] bool ParseString(std::string& out);
  [[nodiscard]] bool ParseNumber(double& out);
  [[nodiscard]] bool ParseUnicodeEscape(std::string& out);
  [[nodiscard]] bool ParseHexQuad(std::uint32_t& out);
  static void AppendUtf8(std::uint32_t code_point, std::string& out);
  void SkipWhitespace();
  [[nodiscard]] bool StartsWith(std::string_view token) const;
  [[nodiscard]] bool Consume(char expected);
  [[nodiscard]] char Advance();
  [[nodiscard]] char Peek() const;
  [[nodiscard]] bool AtEnd() const;
  [[nodiscard]] static bool IsDigit(char c);
  [[nodiscard]] bool Fail(std::string message);

  std::string_view input_;
  std::size_t max_input_bytes_;
  std::size_t max_nesting_depth_;
  std::size_t position_{0};
  std::string error_;
};

[[nodiscard]] RIVACORE_API Status StrictKeys(const JsonValue& value,
                                             const std::set<std::string>& allowed,
                                             const std::string& object_name,
                                             bool allow_unknown_fields);

[[nodiscard]] RIVACORE_API const JsonValue* FindField(const JsonValue& object,
                                                      const std::string& key);

[[nodiscard]] RIVACORE_API Status RequireType(const JsonValue* value, JsonValue::Type type,
                                              const std::string& name);

[[nodiscard]] RIVACORE_API Status NumberToUint64(double value, std::uint64_t& out,
                                                 const std::string& name);

[[nodiscard]] RIVACORE_API Status NumberToSize(double value, std::size_t& out,
                                               const std::string& name);

[[nodiscard]] RIVACORE_API Status ReadOptionalString(const JsonValue& object,
                                                     const std::string& key, std::string& out);

[[nodiscard]] RIVACORE_API Status ReadOptionalNumber(const JsonValue& object,
                                                     const std::string& key, double& out);

}  // namespace riva
