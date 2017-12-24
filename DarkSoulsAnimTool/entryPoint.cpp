#include "structs.hpp"
#include "funcs.hpp"
#include "cxxopts.hpp"

#include <iostream>
#include <algorithm>
#include <queue>

const char commandsString[] =
   "Commands: \n"
   "* scaleAnim [animSearchKey, speedMult]   - Scale the animation speed \n"
   "* importTae                              - Convert TAE to JSON \n"
   "* exportTae                              - Convert JSON to TAE \n"
   " \n"
   "Example: \n"
   ">dsanimtool -i InputTae.tae -o OutputJson.json scaleanim 3005 1.5 \n"
;

void checkEmpty(std::queue<std::wstring>& words) {
   if (words.empty()) {
      printf("Not enough args\n");
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

std::wstring otherString(std::wstring& other, const char* arg) {
   if (other.empty()) {
      printf("Missing required arg: %s \n", arg);
      exit(1);
   }

   return other;
}

std::wstring otherOptionalString(std::wstring other, std::wstring def = L"") {
   if (other.empty()) {
      other = def;
   }

   return other;
}

int main(int argCount, char** args) {
   std::string inputFileArg;
   std::string outputFileArg;
   bool sortEvents = false;

   cxxopts::Options options(args[0]);
   {
      options.custom_help("[OPTION]... [COMMAND] [ARGS]...");
      options.add_options()
         ("help", "Print help")
         ("i, input", "Input file", cxxopts::value(inputFileArg), "PATH")
         ("o, output", "Output file", cxxopts::value(outputFileArg), "PATH")
         (
            "s, sortEvents",
            "Sort events by their type number when outputting JSON",
            cxxopts::value(sortEvents)
         )
      ;

      try {
         auto result = options.parse(argCount, args);

         if (result.count("help")) {
            printf("%s\n", options.help().c_str());
            printf("%s", commandsString);
            exit(0);
         }
      }catch (const cxxopts::OptionException& e){
         printf("error parsing options: %s\n", e.what());
         exit(1);
      }
   }

   std::wstring inputFile = utf8ToUtf16(inputFileArg);
   std::wstring outputFile = utf8ToUtf16(outputFileArg);

   std::queue<std::wstring> words;
   for (int n = 1; n < argCount; ++n) {
      words.push(utf8ToUtf16(args[n]));
   }

   std::wstring command = popString(words);
   std::transform(command.begin(), command.end(), command.begin(), towlower);

   if (command == L"scaleanim") {
      auto s1 = otherString(inputFile, "input");
      auto s2 = otherOptionalString(outputFile, inputFile);
      auto s3 = popString(words);
      float f1 = popFloat(words);
      scaleAnim(s1, s2, s3, f1);
   }else if (command == L"importtae") {
      auto s1 = otherString(inputFile, "input");
      auto s2 = otherOptionalString(outputFile, inputFile + L".json");
      auto b2 = sortEvents;
      importTae(s1, s2, b2);
   }else if (command == L"exporttae") {
      auto s1 = otherString(inputFile, "input");
      auto s2 = otherOptionalString(outputFile);

      if (s2.empty()) {
         std::wstring dir, fileName;
         getPathInfo(s1, dir, fileName);
         stringReplace(fileName, L".json", L"");
         stringReplace(fileName, L".tae", L"");
         s2 = fileName + L".tae";
      }

      exportTae(s1, s2);
   }else {
      printf("%s\n", options.help().c_str());
      printf("%s", commandsString);
   }

   return 0;
}
