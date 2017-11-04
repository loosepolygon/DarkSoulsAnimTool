#pragma once

#include <string>

struct TaeFile;

namespace json {
   class JSON;
}

// readTaeFile.cpp

TaeFile* readTaeFile(std::wstring sourceTaePath);
TaeFile* readTaeFile(FILE* file);

// taeToJson.cpp

json::JSON taeToJson(TaeFile* taeFile);

// tools.cpp

void scaleAnim(
   std::wstring sourceTaePath,
   std::wstring animFileName,
   float scale
);

void exportTae(std::wstring sourceTaePath, std::wstring outputDir);

// utility.cpp

std::string utf16ToUtf8(std::wstring inputText);
