#include "structs.hpp"
#include "funcs.hpp"
#include "json.hpp"
#include "tinyxml2.h"

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

   wprintf_s(L"Scaling anim... \n");

   stringReplace(foundAnim, L".hkxwin", L".hkx");

   std::wstring dir, fileName;
   getPathInfo(sourceTaePath, dir, fileName);
   std::wstring hkxwin32Dir = getFullPath(dir + L"../../hkxwin32/");
   std::wstring animPath = hkxwin32Dir + foundAnim;
   
   if (fileExists(animPath) == false) {
      wprintf_s(L"Could not find anim file: %s \n", animPath.c_str());
      exit(1);
   }

   std::wstring intermediateDir = hkxwin32Dir + L"DSAnimTool-temp/";
   _wmkdir(intermediateDir.c_str());

   std::wstring executeText(256, L'\0');
   std::wstring xmlPath = intermediateDir + foundAnim + L".xml";
   swprintf(
      &executeText[0],
      executeText.size(),
      L"hkxcmd convert -i \"%s\" -o \"%s\" -v:XML -f SAVE_TEXT_FORMAT^|SAVE_TEXT_NUMBERS",
      animPath.c_str(),
      xmlPath.c_str()
   );

   int returnCode = _wsystem(executeText.c_str());
   if (returnCode != 0) {
      exit(1);
   }

   // ---------

   using namespace tinyxml2;

   XMLDocument xml;
   xml.LoadFile(utf16ToUtf8(xmlPath).c_str());

   auto parseError = [](const char* text) {
      printf_s("Error parsing xml file: %s \n", text);
      exit(1);
   };

   auto findAll = [](
      XMLElement* rootElement,
      const char* key,
      const char* value
   ) -> std::vector<XMLElement*> {
      std::vector<XMLElement*> result;

      XMLElement* e = rootElement->FirstChildElement();
      while (e) {
         if (e->Attribute(key, value)) {
            result.push_back(e);
         }

         e = e->NextSiblingElement();
      }

      return result;
   };

   XMLElement* data = xml.FirstChildElement("hkpackfile")->FirstChildElement("hksection");
   if (!data->Attribute("name", "__data__")) {
      parseError("Bad __data__");
   }

   for (XMLElement* anim : findAll(data, "class", "hkaSplineCompressedAnimation")) {
      int trackCount = 0;
      std::vector<byte> bytes;
      {
         XMLElement* trackCountElement = findAll(anim, "name", "numberOfTransformTracks")[0];
         std::string text = trackCountElement->GetText();
         trackCount = atoi(text.c_str());

         XMLElement* animData = findAll(anim, "name", "data")[0];
         text = animData->GetText();
         char* textStart = nullptr;
         for (char& c : text) {
            bool isNumber = c >= '0' && c <= '9';
            if (textStart && isNumber == false) {
               c = '\0';

               int num = atoi(textStart);
               bytes.push_back((byte)num);

               textStart = nullptr;
            }else if (textStart == nullptr && isNumber) {
               textStart = &c;
            }
         }
      }

      // Debug output
      //FILE* file = fopen("havok SCA data.bin", "wb");
      //fwrite(bytes.data(), 1, bytes.size(), file);
      //fclose(file);

      SCA::SCAData* scaData = readSCAData(trackCount, bytes);

      // int newFrameCount = -1;
      // std::vector<SCA::Frame> frames = getFrames(scaData, newFrameCount);
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
