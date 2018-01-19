#pragma once

#include "structs.hpp"

#include <string>
#include <vector>

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
json::JSON taeToJson(TaeFile* taeFile, bool sortEventsByType);

// taeToJson.cpp
TaeFile* jsonToTae(json::JSON root);

// sca.cpp
SCA::SCAData* readSCAData(int trackCount, const std::vector<byte>& bytes);
void getFrames(Anims::Animation* animation, SCA::SCAData* scaData);
void writeFramesAsSCA(Anims::Animation* animation, std::vector<byte>& bytes);

// tools.cpp
void scaleAnim(
   std::wstring sourceTaePath,
   std::wstring destTaePath,
   std::wstring animFileName,
   float scale
);
void scaleAnimEx(
   std::wstring sourceTaePath,
   std::wstring destTaePath,
   std::wstring animFileName,
   float scale
);
void importTae(std::wstring sourceTaePath, std::wstring destJsonPath, bool sortEvents);
void exportTae(std::wstring sourceJsonPath, std::wstring destTaePath);

// utility.cpp
void getPathInfo(const std::wstring& path, std::wstring& dir, std::wstring& fileName);
std::wstring getFullPath(const std::wstring& path);
void createBackupFile(const std::wstring& path);
bool fileExists(const std::wstring& path);
void hkxcmdHkxToXml(const std::wstring& hkx, const std::wstring& xml);
void hkxcmdXmlToHkx(const std::wstring& xml, const std::wstring& hkx);
void stringReplace(
   std::wstring& text,
   const std::wstring& from,
   const std::wstring& to
);
std::wstring utf8ToUtf16(std::string inputText);
std::string utf16ToUtf8(std::wstring inputText);

// Inline headers
#include "dataReading.inl"
#include "dataWriting.inl"
