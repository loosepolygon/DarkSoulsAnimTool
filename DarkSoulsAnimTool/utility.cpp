#include "stdafx.h"

#include "funcs.hpp"

#include "UTF8-CPP\checked.h"

#include <algorithm>

std::string utf16ToUtf8(std::wstring inputText) {
   std::string result;

   utf8::utf16to8(
      inputText.c_str(),
      inputText.c_str() + inputText.length(),
      std::back_inserter(result)
   );

   return result;
}