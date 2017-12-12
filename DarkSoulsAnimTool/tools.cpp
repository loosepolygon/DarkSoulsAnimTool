#include "structs.hpp"
#include "funcs.hpp"
#include "json.hpp"

#include <string>
#include <cstdio>
#include <vector>
#include <algorithm>

void scaleAnim(
   std::wstring sourceTaePath,
   std::wstring destTaePath,
   std::wstring animSearchKey,
   float speedMult
) {
   createBackupFile(sourceTaePath);

   std::transform(animSearchKey.begin(), animSearchKey.end(), animSearchKey.begin(), towlower);

   FILE* file = _wfopen(sourceTaePath.c_str(), L"rb");
   if (file == nullptr) {
      wprintf_s(L"Cannot open file: %s\n", sourceTaePath.c_str());
      exit(1);
   }

   TaeFile* taeFile = readTaeFile(file);

   fclose(file);

   bool found = false;
   std::vector<Event>* events = nullptr;
   std::wstring foundAnim;
   for (size_t n = 0; n < taeFile->animData.size(); ++n) {
      std::wstring animName = taeFile->animData[n].animFile.name;
      std::transform(animName.begin(), animName.end(), animName.begin(), towlower);

      found = animName.find(animSearchKey) != std::wstring::npos;

      if (found) {
         foundAnim = animName;

         events = &taeFile->animData[n].events;

         break;
      }
   }

   if (found == false) {
      wprintf_s(L"Did not find \"%s\" in %s \n", animSearchKey.c_str(), sourceTaePath.c_str());
      exit(1);
   }

   wprintf_s(L"Found anim \"%s\" in %s \n", foundAnim.c_str(), sourceTaePath.c_str());

   wprintf_s(L"Scaling events... \n");

   for (Event& event : *events) {
      // Don't scale sounds, it cuts off the sound early and sounds awful
      bool shouldScaleDuration = !(event.type == 128 || event.type == 129);

      if (shouldScaleDuration) {
         event.beginTime /= speedMult;
         event.endTime /= speedMult;
      }else{
         float duration = event.endTime - event.beginTime;
         event.beginTime /= speedMult;
         event.endTime = event.beginTime + duration;
      }
   }

   writeTaeFile(destTaePath, taeFile);
}

void importTae(std::wstring sourceTaePath, std::wstring destJsonPath, bool sortEvents) {
   TaeFile* taeFile = readTaeFile(sourceTaePath);
   json::JSON root = taeToJson(taeFile, sortEvents);
   std::string jsonText = root.dump();

   wprintf_s(L"Importing %s as %s...\n", sourceTaePath.c_str(), destJsonPath.c_str());
   FILE* file = _wfopen(destJsonPath.c_str(), L"wb");

   if (file == NULL) {
      wprintf_s(L"Cannot open file for writing: %s\n", destJsonPath.c_str());
      return;
   }

   fwrite(jsonText.c_str(), 1, jsonText.size(), file);

   fclose(file);
}

void exportTae(std::wstring sourceJsonPath, std::wstring destTaePath) {
   std::string jsonText;
   {
      FILE* file = _wfopen(sourceJsonPath.c_str(), L"rb");
      if (file == NULL) {
         wprintf_s(L"Cannot open file: %s\n", sourceJsonPath.c_str());
         return;
      }

      fseek(file, 0, SEEK_END);
      long fileSize = ftell(file);
      fseek(file, 0, SEEK_SET);

      jsonText.resize(fileSize);
      fread((void*)jsonText.data(), 1, fileSize, file);
      fclose(file);
   }

   json::JSON root = json::JSON::Load(jsonText);
   TaeFile* taeFile = jsonToTae(root);
   writeTaeFile(destTaePath, taeFile);
}
