#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include <set>

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

class JsonParser {
 public:
  explicit JsonParser(std::string_view input);

  [[nodiscard]] bool Parse(JsonValue& value);
  [[nodiscard]] const std::string& error() const noexcept { return error_; }

 private:
  [[nodiscard]] bool ParseValue(JsonValue& value);
  [[nodiscard]] bool ParseObject(JsonValue& value);
  [[nodiscard]] bool ParseArray(JsonValue& value);
  [[nodiscard]] bool ParseString(std::string& out);
  [[nodiscard]] bool ParseNumber(double& out);
  void SkipWhitespace();
  [[nodiscard]] bool StartsWith(std::string_view token) const;
  [[nodiscard]] bool Consume(char expected);
  [[nodiscard]] char Advance();
  [[nodiscard]] char Peek() const;
  [[nodiscard]] bool AtEnd() const;
  [[nodiscard]] static bool IsDigit(char c);
  [[nodiscard]] bool Fail(std::string message);

  std::string_view input_;
  std::size_t position_{0};
  std::string error_;
};

[[nodiscard]] Status StrictKeys(
    const JsonValue& value,
    const std::set<std::string>& allowed,
    const std::string& object_name,
    bool allow_unknown_fields);

[[nodiscard]] const JsonValue* FindField(const JsonValue& object, const std::string& key);

[[nodiscard]] Status RequireType(const JsonValue* value, JsonValue::Type type, const std::string& name);

[[nodiscard]] Status NumberToUint64(double value, std::uint64_t& out, const std::string& name);

[[nodiscard]] Status NumberToSize(double value, std::size_t& out, const std::string& name);

[[nodiscard]] Status ReadOptionalString(
    const JsonValue& object,
    const std::string& key,
    std::string& out);

[[nodiscard]] Status ReadOptionalNumber(
    const JsonValue& object,
    const std::string& key,
    double& out);

} // namespace riva
