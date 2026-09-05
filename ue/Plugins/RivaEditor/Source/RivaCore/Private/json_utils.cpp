#include "riva/json_utils.hpp"

#include <cmath>
#include <limits>

namespace riva {

JsonParser::JsonParser(std::string_view input, std::size_t max_input_bytes,
                       std::size_t max_nesting_depth)
    : input_(input), max_input_bytes_(max_input_bytes), max_nesting_depth_(max_nesting_depth) {}

bool JsonParser::Parse(JsonValue& value) {
  if (input_.size() > max_input_bytes_) {
    return Fail("JSON input exceeds configured size limit");
  }
  SkipWhitespace();
  if (!ParseValue(value, 0)) return false;
  SkipWhitespace();
  if (position_ != input_.size()) return Fail("unexpected trailing content");
  return true;
}

bool JsonParser::ParseValue(JsonValue& value, std::size_t depth) {
  SkipWhitespace();
  if (AtEnd()) return Fail("unexpected end of JSON");
  if (depth > max_nesting_depth_) return Fail("JSON nesting exceeds configured depth limit");

  const char c = Peek();
  if (c == '{') return ParseObject(value, depth);
  if (c == '[') return ParseArray(value, depth);
  if (c == '"') {
    value.type = JsonValue::Type::kString;
    return ParseString(value.string_value);
  }
  if (c == '-' || IsDigit(c)) {
    value.type = JsonValue::Type::kNumber;
    return ParseNumber(value.number_value);
  }
  if (StartsWith("true")) {
    position_ += 4;
    value.type = JsonValue::Type::kBool;
    value.bool_value = true;
    return true;
  }
  if (StartsWith("false")) {
    position_ += 5;
    value.type = JsonValue::Type::kBool;
    value.bool_value = false;
    return true;
  }
  if (StartsWith("null")) {
    position_ += 4;
    value.type = JsonValue::Type::kNull;
    return true;
  }
  return Fail("invalid JSON value");
}

bool JsonParser::ParseObject(JsonValue& value, std::size_t depth) {
  value.type = JsonValue::Type::kObject;
  (void)Consume('{');
  SkipWhitespace();

  if (!AtEnd() && Peek() == '}') {
    (void)Consume('}');
    return true;
  }

  while (!AtEnd()) {
    std::string key;
    if (!ParseString(key)) return false;
    SkipWhitespace();
    if (!Consume(':')) return Fail("expected ':' after object key");

    JsonValue child;
    if (!ParseValue(child, depth + 1)) return false;

    if (value.object_value.find(key) != value.object_value.end()) {
      return Fail("duplicate object key: " + key);
    }
    value.object_value.emplace(std::move(key), std::move(child));
    SkipWhitespace();

    if (Consume('}')) return true;
    if (!Consume(',')) return Fail("expected ',' or '}' in object");
    SkipWhitespace();
  }
  return Fail("unterminated object");
}

bool JsonParser::ParseArray(JsonValue& value, std::size_t depth) {
  value.type = JsonValue::Type::kArray;
  (void)Consume('[');
  SkipWhitespace();

  if (!AtEnd() && Peek() == ']') {
    (void)Consume(']');
    return true;
  }

  while (!AtEnd()) {
    JsonValue child;
    if (!ParseValue(child, depth + 1)) return false;
    value.array_value.push_back(std::move(child));
    SkipWhitespace();

    if (Consume(']')) return true;
    if (!Consume(',')) return Fail("expected ',' or ']' in array");
    SkipWhitespace();
  }
  return Fail("unterminated array");
}

bool JsonParser::ParseString(std::string& out) {
  if (!Consume('"')) return Fail("expected string");
  out.clear();
  while (!AtEnd()) {
    const char c = Advance();
    if (c == '"') return true;
    if (c == '\\') {
      if (AtEnd()) return Fail("unterminated string escape");
      const char escaped = Advance();
      switch (escaped) {
        case '"':
        case '\\':
        case '/':
          out.push_back(escaped);
          break;
        case 'b':
          out.push_back('\b');
          break;
        case 'f':
          out.push_back('\f');
          break;
        case 'n':
          out.push_back('\n');
          break;
        case 'r':
          out.push_back('\r');
          break;
        case 't':
          out.push_back('\t');
          break;
        case 'u':
          if (!ParseUnicodeEscape(out)) return false;
          break;
        default:
          return Fail("unsupported string escape");
      }
      continue;
    }
    if (static_cast<unsigned char>(c) < 0x20) {
      return Fail("control character inside string");
    }
    out.push_back(c);
  }
  return Fail("unterminated string");
}

bool JsonParser::ParseUnicodeEscape(std::string& out) {
  std::uint32_t code_point = 0;
  if (!ParseHexQuad(code_point)) {
    return false;
  }

  if (code_point >= 0xD800U && code_point <= 0xDBFFU) {
    if (AtEnd() || Advance() != '\\' || AtEnd() || Advance() != 'u') {
      return Fail("high surrogate must be followed by a low surrogate");
    }
    std::uint32_t low_surrogate = 0;
    if (!ParseHexQuad(low_surrogate)) {
      return false;
    }
    if (low_surrogate < 0xDC00U || low_surrogate > 0xDFFFU) {
      return Fail("high surrogate must be followed by a low surrogate");
    }
    code_point = 0x10000U + ((code_point - 0xD800U) << 10U) + (low_surrogate - 0xDC00U);
  } else if (code_point >= 0xDC00U && code_point <= 0xDFFFU) {
    return Fail("unexpected low surrogate");
  }

  AppendUtf8(code_point, out);
  return true;
}

bool JsonParser::ParseHexQuad(std::uint32_t& out) {
  out = 0;
  for (int Index = 0; Index < 4; ++Index) {
    if (AtEnd()) {
      return Fail("incomplete Unicode escape");
    }
    const char Digit = Advance();
    std::uint32_t Value = 0;
    if (Digit >= '0' && Digit <= '9') {
      Value = static_cast<std::uint32_t>(Digit - '0');
    } else if (Digit >= 'a' && Digit <= 'f') {
      Value = static_cast<std::uint32_t>(Digit - 'a' + 10);
    } else if (Digit >= 'A' && Digit <= 'F') {
      Value = static_cast<std::uint32_t>(Digit - 'A' + 10);
    } else {
      return Fail("invalid Unicode escape");
    }
    out = (out << 4U) | Value;
  }
  return true;
}

void JsonParser::AppendUtf8(std::uint32_t code_point, std::string& out) {
  if (code_point <= 0x7FU) {
    out.push_back(static_cast<char>(code_point));
  } else if (code_point <= 0x7FFU) {
    out.push_back(static_cast<char>(0xC0U | (code_point >> 6U)));
    out.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
  } else if (code_point <= 0xFFFFU) {
    out.push_back(static_cast<char>(0xE0U | (code_point >> 12U)));
    out.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
  } else {
    out.push_back(static_cast<char>(0xF0U | (code_point >> 18U)));
    out.push_back(static_cast<char>(0x80U | ((code_point >> 12U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
  }
}

bool JsonParser::ParseNumber(double& out) {
  const std::size_t start = position_;
  if (Peek() == '-') (void)Advance();
  if (AtEnd()) return Fail("invalid number");

  if (Peek() == '0') {
    (void)Advance();
  } else if (IsDigit(Peek())) {
    while (!AtEnd() && IsDigit(Peek())) (void)Advance();
  } else {
    return Fail("invalid number");
  }

  if (!AtEnd() && Peek() == '.') {
    (void)Advance();
    if (AtEnd() || !IsDigit(Peek())) return Fail("invalid number fraction");
    while (!AtEnd() && IsDigit(Peek())) (void)Advance();
  }

  if (!AtEnd() && (Peek() == 'e' || Peek() == 'E')) {
    (void)Advance();
    if (!AtEnd() && (Peek() == '+' || Peek() == '-')) (void)Advance();
    if (AtEnd() || !IsDigit(Peek())) return Fail("invalid number exponent");
    while (!AtEnd() && IsDigit(Peek())) (void)Advance();
  }

  try {
    out = std::stod(std::string(input_.substr(start, position_ - start)));
  } catch (...) {
    return Fail("number conversion failed");
  }
  if (!std::isfinite(out)) return Fail("number must be finite");
  return true;
}

void JsonParser::SkipWhitespace() {
  while (!AtEnd()) {
    const char c = Peek();
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
      (void)Advance();
      continue;
    }
    break;
  }
}

bool JsonParser::StartsWith(std::string_view token) const {
  return input_.substr(position_, token.size()) == token;
}

bool JsonParser::Consume(char expected) {
  if (!AtEnd() && Peek() == expected) {
    ++position_;
    return true;
  }
  return false;
}

char JsonParser::Advance() { return input_[position_++]; }
char JsonParser::Peek() const { return input_[position_]; }
bool JsonParser::AtEnd() const { return position_ >= input_.size(); }
bool JsonParser::IsDigit(char c) { return c >= '0' && c <= '9'; }
bool JsonParser::Fail(std::string message) {
  error_ = std::move(message);
  return false;
}

Status StrictKeys(const JsonValue& value, const std::set<std::string>& allowed,
                  const std::string& object_name, bool allow_unknown_fields) {
  if (allow_unknown_fields) return Status::Ok();
  for (const auto& [key, unused] : value.object_value) {
    (void)unused;
    if (allowed.find(key) == allowed.end()) {
      std::string message = "unknown key in ";
      message.append(object_name).append(": ").append(key);
      return Status(StatusCode::kInvalidArgument, std::move(message));
    }
  }
  return Status::Ok();
}

const JsonValue* FindField(const JsonValue& object, const std::string& key) {
  const auto found = object.object_value.find(key);
  if (found == object.object_value.end()) return nullptr;
  return &found->second;
}

Status RequireType(const JsonValue* value, JsonValue::Type type, const std::string& name) {
  if (value == nullptr) {
    return Status(StatusCode::kInvalidArgument, "missing required field: " + name);
  }
  if (value->type != type) {
    return Status(StatusCode::kInvalidArgument, "invalid type for field: " + name);
  }
  return Status::Ok();
}

Status NumberToUint64(double value, std::uint64_t& out, const std::string& name) {
  const double exclusive_upper_bound = std::ldexp(1.0, std::numeric_limits<std::uint64_t>::digits);
  if (value < 0.0 || std::floor(value) != value || value >= exclusive_upper_bound) {
    return Status(StatusCode::kInvalidArgument, "field must be a non-negative integer: " + name);
  }
  out = static_cast<std::uint64_t>(value);
  return Status::Ok();
}

Status NumberToSize(double value, std::size_t& out, const std::string& name) {
  const double exclusive_upper_bound = std::ldexp(1.0, std::numeric_limits<std::size_t>::digits);
  if (value < 0.0 || std::floor(value) != value || value >= exclusive_upper_bound) {
    return Status(StatusCode::kInvalidArgument, "field must be a non-negative integer: " + name);
  }
  out = static_cast<std::size_t>(value);
  return Status::Ok();
}

Status ReadOptionalString(const JsonValue& object, const std::string& key, std::string& out) {
  const JsonValue* value = FindField(object, key);
  if (value == nullptr) return Status::Ok();
  if (value->type != JsonValue::Type::kString) {
    return Status(StatusCode::kInvalidArgument, "invalid type for field: " + key);
  }
  out = value->string_value;
  return Status::Ok();
}

Status ReadOptionalNumber(const JsonValue& object, const std::string& key, double& out) {
  const JsonValue* value = FindField(object, key);
  if (value == nullptr) return Status::Ok();
  if (value->type != JsonValue::Type::kNumber) {
    return Status(StatusCode::kInvalidArgument, "invalid type for field: " + key);
  }
  out = value->number_value;
  return Status::Ok();
}

}  // namespace riva
