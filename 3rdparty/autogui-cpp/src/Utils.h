#pragma once

#include <cctype>
#include <chrono>
#include <cstring>
#include <thread>

namespace Robot {

void delay(unsigned int ms);

namespace KeyUtils {
// 检查字符是否需要Shift键
inline bool NeedsShift(char c) {
  // 大写字母需要Shift
  if (c >= 'A' && c <= 'Z') {
    return true;
  }

  // 特殊符号需要Shift
  const char* shiftChars = "!@#$%^&*()_+{}|:\"<>?~";
  return (strchr(shiftChars, c) != nullptr);
}

// 获取字符的基础键（物理键位）
inline char GetBaseKey(char c) {
  // 对于字母，返回小写形式
  if (c >= 'A' && c <= 'Z') {
    return std::tolower(c);
  }

  // 对于数字，保持不变
  if (c >= '0' && c <= '9') {
    return c;
  }

  // 对于需要Shift的符号，返回对应的基础键
  switch (c) {
  case '!': return '1';
  case '@': return '2';
  case '#': return '3';
  case '$': return '4';
  case '%': return '5';
  case '^': return '6';
  case '&': return '7';
  case '*': return '8';
  case '(': return '9';
  case ')': return '0';
  case '_': return '-';
  case '+': return '=';
  case '{': return '[';
  case '}': return ']';
  case '|': return '\\';
  case ':': return ';';
  case '"': return '\'';
  case '<': return ',';
  case '>': return '.';
  case '?': return '/';
  case '~': return '`';
  default: return c;
  }
}

// 检查字符是否是有效的ASCII字符
inline bool IsValidAscii(char c) {
  return c >= 32 && c <= 126;  // 可打印ASCII字符
}

} // namespace KeyUtils

} // namespace Robot
