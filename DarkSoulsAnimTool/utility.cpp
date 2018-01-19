#include "funcs.hpp"
#include "structs.hpp"

#include "UTF8-CPP\checked.h"

#include <algorithm>
#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "Shlwapi.h"

void getPathInfo(const std::wstring& path, std::wstring& dir, std::wstring& fileName){
   dir = L"./";
   fileName = path;

   for (int n = path.size(); n --> 0;) {
      if (path[n] == L'/' || path[n] == L'\\') {
         dir = path.substr(0, n + 1);
         fileName = &path[n + 1];
         break;
      }
   }
}

std::wstring getFullPath(const std::wstring& path) {
   wchar_t buffer[MAX_PATH];
   GetFullPathNameW(path.c_str(), MAX_PATH, buffer, NULL);
   return buffer;
}

void createBackupFile(const std::wstring& path) {
   FILE* file = _wfopen(path.c_str(), L"rb");

   if (file == nullptr) {
      // wprintf_s(L"Cannot open file: %s\n", path.c_str());
      // exit(1);
      return;
   }

   fseek(file, 0, SEEK_END);
   long fileSize = ftell(file);
   fseek(file, 0, SEEK_SET);

   std::wstring backupPath = path + L".bak";
   if (fileExists(backupPath)) {
      fclose(file);

      return;
   }
   FILE* backupFile = _wfopen(backupPath.c_str(), L"wb");

   byte* bytes = new byte[fileSize];
   fread(bytes, 1, fileSize, file);

   fclose(file);

   fwrite(bytes, 1, fileSize, backupFile);

   delete bytes;

   wprintf_s(L"Wrote backup file: %s\n", backupPath.c_str());

   fclose(backupFile);
}

bool fileExists(const std::wstring& path) {
   FILE* file = _wfopen(path.c_str(), L"rb");
   if (file) {
      fclose(file);
      return true;
   }
   else {
      return false;
   }
}

wchar_t textToExecute[1024];

void hkxcmdHkxToXml(const std::wstring& hkx, const std::wstring& xml){
   swprintf(
      &textToExecute[0],
      sizeof(textToExecute),
      L"hkxcmd convert -i \"%s\" -o \"%s\" -v:XML -f SAVE_TEXT_FORMAT^|SAVE_TEXT_NUMBERS",
      hkx.c_str(),
      xml.c_str()
   );

   int returnCode = _wsystem(textToExecute);
   if (returnCode != 0) {
      exit(1);
   }
}

void hkxcmdXmlToHkx(const std::wstring& xml, const std::wstring& hkx){
   swprintf(
      &textToExecute[0],
      sizeof(textToExecute),
      L"hkxcmd convert -i \"%s\" -o \"%s\" -f SAVE_DEFAULT",
      xml.c_str(),
      hkx.c_str()
   );

   int returnCode = _wsystem(textToExecute);
   if (returnCode != 0) {
      exit(1);
   }
}

void stringReplace(std::wstring& text, const std::wstring& from, const std::wstring& to) {
   size_t startPos = text.find(from);
   if (startPos != std::wstring::npos) {
      text.replace(startPos, from.size(), to);
   }
}

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
