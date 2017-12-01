#pragma once

#include <string>

struct TaeFile;

namespace json {
   class JSON;
}

// readTaeFile.cpp
TaeFile* readTaeFile(std::wstring sourceTaePath);
TaeFile* readTaeFile(FILE* file);
int getEventParamCount(int eventType);

// writeTaeFile.cpp
void writeTaeFile(std::wstring outputPath, TaeFile* taeFile);

// taeToJson.cpp
json::JSON taeToJson(TaeFile* taeFile);

// taeToJson.cpp
TaeFile* jsonToTae(json::JSON root);

// tools.cpp
void scaleAnim(
   std::wstring sourceTaePath,
   std::wstring animFileName,
   float scale
);
void importTae(std::wstring sourceTaePath, std::wstring outputDir);
void exportTae(std::wstring sourceJsonPath, std::wstring outputDir);

// utility.cpp
std::wstring getBaseFileName(std::wstring path);
void stringReplace(
   std::wstring& text,
   const std::wstring& from,
   const std::wstring& to
);
std::wstring utf8ToUtf16(std::string inputText);
std::string utf16ToUtf8(std::wstring inputText);
