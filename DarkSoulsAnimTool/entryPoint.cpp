#include "structs.hpp"
#include "funcs.hpp"

#include <iostream>
#include <algorithm>
#include <queue>

const char usageString[] =
   "Commands: \n"
   "* scaleAnim taePath animSearchKey speedMult   - Scale the animation speed \n"
   "* importTae taePath [outputDir]               - Convert TAE to JSON \n"
   "* exportTae jsonPath [outputDir]              - Convert JSON to TAE \n"
;

void printUsage() {
   printf("\n%s\n", usageString);
}

void checkEmpty(std::queue<std::wstring>& words) {
   if (words.empty()) {
      printf("Not enough args\n");
      printUsage();
      exit(1);
   }
}

std::wstring popString(std::queue<std::wstring>& words) {
   checkEmpty(words);

   std::wstring result = words.front();
   words.pop();
   return result;
}

std::wstring popOptionalString(std::queue<std::wstring>& words, const std::wstring& default) {
   if (words.empty()) {
      return default;
   }else{
      return popString(words);
   }
}

float popFloat(std::queue<std::wstring>& words) {
   checkEmpty(words);

   float result = (float)_wtof(words.front().c_str());
   words.pop();
   return result;
}

int wmain(int argCount, const wchar_t** args){
   std::queue<std::wstring> words;
   for (int n = 1; n < argCount; ++n) {
      words.push(args[n]);
   }

   checkEmpty(words);

   std::wstring command = popString(words);
   std::transform(command.begin(), command.end(), command.begin(), towlower);

   for (int n = 0; n < argCount; ++n) {
      if (command == L"scaleanim") {
         /*scaleAnim(
            popString(words),
            popString(words),
            popFloat(words)
         );*/
         // C++ provides no guarantee that the two popStrings will be evaluated before the
         // popFloat, it's undefined. I hate this language so much.

         auto s1 = popString(words);
         auto s2 = popString(words);
         float f1 = popFloat(words);
         scaleAnim(s1, s2, f1);

         break;
      }else if (command == L"importtae") {
         auto s1 = popString(words);
         auto s2 = popOptionalString(words, L".");
         importTae(s1, s2);

         break;
      }else if (command == L"exporttae") {
         auto s1 = popString(words);
         auto s2 = popOptionalString(words, L".");
         exportTae(s1, s2);

         break;
      }else {
         printUsage();

         break;
      }
   }

   return 0;
}
