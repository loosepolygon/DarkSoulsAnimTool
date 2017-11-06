#include "funcs.hpp"

#include "UTF8-CPP\checked.h"

#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

std::wstring utf8ToUtf16(std::string inputText) {
   std::wstring result;

   utf8::utf8to16(
      inputText.begin(),
      inputText.end(),
      std::back_inserter(result)
   );

   return result;
}

std::string utf16ToUtf8(std::wstring inputText) {
   int utf8Size = WideCharToMultiByte(
      CP_UTF8,
      0,
      &inputText[0],
      (int)inputText.size(),
      NULL, 0, NULL, NULL
   );
   std::string result(utf8Size, 0);
   WideCharToMultiByte(
      CP_UTF8,
      0,
      &inputText[0],
      (int)inputText.size(),
      &result[0],
      utf8Size,
      NULL, NULL
   );

   return result;
}
