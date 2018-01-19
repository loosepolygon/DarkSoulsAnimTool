#include "structs.hpp"
#include "funcs.hpp"
#include "json.hpp"
#include "tinyxml2.h"

#include <string>
#include <cstdio>
#include <vector>
#include <algorithm>

// TESTING
#include <sstream>
#include <iomanip>

using namespace tinyxml2;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Anim scaling

struct ScaleAnimSharedData{
   TAE::TaeFile* taeFile = nullptr;
   std::vector<TAE::Event>* taeEvents = nullptr;

   std::wstring sourceAnimPath;
   std::wstring destAnimPath;
   std::wstring xmlPath;

   XMLDocument xml;
   XMLElement* scaElement;
   XMLElement* motionElement;
};

std::vector<XMLElement*> findAll(
   XMLElement* rootElement,
   const char* key,
   const char* value
){
   std::vector<XMLElement*> result;

   XMLElement* e = rootElement->FirstChildElement();
   while (e) {
      if (e->Attribute(key, value)) {
         result.push_back(e);
      }

      e = e->NextSiblingElement();
   }

   return result;
}

void parseError(const char* text) {
   printf_s("Error parsing xml file: %s \n", text);
   exit(1);
};

ScaleAnimSharedData* scaleAnimShared(
   std::wstring sourceTaePath,
   std::wstring destTaePath,
   std::wstring animSearchKey
){
   ScaleAnimSharedData* shared = new ScaleAnimSharedData;

   std::transform(animSearchKey.begin(), animSearchKey.end(), animSearchKey.begin(), towlower);

   FILE* file = _wfopen(sourceTaePath.c_str(), L"rb");
   if (file == nullptr) {
      wprintf_s(L"Cannot open file: %s\n", sourceTaePath.c_str());
      exit(1);
   }

   shared->taeFile = readTaeFile(file);

   fclose(file);

   bool found = false;
   std::wstring foundAnim;
   for (size_t n = 0; n < shared->taeFile->animData.size(); ++n) {
      std::wstring animName = shared->taeFile->animData[n].animFile.name;
      std::transform(animName.begin(), animName.end(), animName.begin(), towlower);

      found = animName.find(animSearchKey) != std::wstring::npos;

      if (found) {
         foundAnim = animName;
         shared->taeEvents = &shared->taeFile->animData[n].events;

         break;
      }
   }

   if (found == false) {
      wprintf_s(L"Did not find \"%s\" in %s \n", animSearchKey.c_str(), sourceTaePath.c_str());
      exit(1);
   }

   wprintf_s(L"Found anim \"%s\" in %s \n", foundAnim.c_str(), sourceTaePath.c_str());

   stringReplace(foundAnim, L".hkxwin", L".hkx");

   {
      std::wstring dir, fileName;
      getPathInfo(sourceTaePath, dir, fileName);
      std::wstring hkxwin32Dir = getFullPath(dir + L"..\\..\\hkxwin32\\");
      shared->sourceAnimPath = hkxwin32Dir + foundAnim;

      if (fileExists(shared->sourceAnimPath) == false) {
         wprintf_s(L"Could not find anim file: %s \n", shared->sourceAnimPath.c_str());
         exit(1);
      }

      std::wstring intermediateDir = hkxwin32Dir + L"DSAnimTool-temp\\";
      _wmkdir(intermediateDir.c_str());

      shared->xmlPath = intermediateDir + foundAnim + L".xml";
   }

   {
      std::wstring dir, fileName;
      getPathInfo(destTaePath, dir, fileName);
      std::wstring hkxwin32Dir = getFullPath(dir + L"..\\..\\hkxwin32\\");
      shared->destAnimPath = hkxwin32Dir + foundAnim;
   }

   hkxcmdHkxToXml(shared->sourceAnimPath.c_str(), shared->xmlPath.c_str());

   // Read XML

   shared->xml.LoadFile(utf16ToUtf8(shared->xmlPath).c_str());

   auto packFileElement = shared->xml.FirstChildElement("hkpackfile");
   auto dataElement = packFileElement->FirstChildElement("hksection");
   if (!dataElement->Attribute("name", "__data__")) {
      parseError("Bad __data__");
   }

   shared->scaElement = findAll(dataElement, "class", "hkaSplineCompressedAnimation")[0];
   shared->motionElement = findAll(dataElement, "class", "hkaDefaultAnimatedReferenceFrame")[0];

   createBackupFile(destTaePath);
   createBackupFile(shared->destAnimPath);

   return shared;
}

void scaleAnim(
   std::wstring sourceTaePath,
   std::wstring destTaePath,
   std::wstring animSearchKey,
   float speedMult
) {
   ScaleAnimSharedData* shared = scaleAnimShared(sourceTaePath, destTaePath, animSearchKey);

   wprintf_s(L"Scaling events... \n");

   {
      for (TAE::Event& event : *shared->taeEvents) {
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

      writeTaeFile(destTaePath, shared->taeFile);
   }

   wprintf_s(L"Scaling anim... \n");

   auto scaleDurationXml = [speedMult](XMLElement* e, bool inverse = false){
      float duration = (float)(atof(e->GetText()));
      if(inverse){
         duration *= speedMult;
      }else{
         duration /= speedMult;
      }
      char buffer[32];
      sprintf(buffer, "%f", duration);
      e->SetText(buffer);
   };
   scaleDurationXml(findAll(shared->motionElement, "name", "duration")[0]);
   scaleDurationXml(findAll(shared->scaElement, "name", "duration")[0]);
   scaleDurationXml(findAll(shared->scaElement, "name", "blockDuration")[0]);
   scaleDurationXml(findAll(shared->scaElement, "name", "blockInverseDuration")[0], true);
   scaleDurationXml(findAll(shared->scaElement, "name", "frameDuration")[0]);

   shared->xml.SaveFile(utf16ToUtf8(shared->xmlPath).c_str());

   hkxcmdXmlToHkx(shared->xmlPath.c_str(), shared->destAnimPath.c_str());
}

void scaleAnimEx(
   std::wstring sourceTaePath,
   std::wstring destTaePath,
   std::wstring animSearchKey,
   float speedMult
) {
   ScaleAnimSharedData* shared = scaleAnimShared(sourceTaePath, destTaePath, animSearchKey);

   wprintf_s(L"Scaling events... \n");

   {
      for (TAE::Event& event : *shared->taeEvents) {
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

      writeTaeFile(destTaePath, shared->taeFile);
   }

   wprintf_s(L"Scaling anim... \n");

   Anims::Animation* animation = new Anims::Animation;
   std::vector<byte> bytes;
   XMLElement* animDataElement;
   {
      std::string text;

      text = findAll(shared->scaElement, "name", "numberOfTransformTracks")[0]->GetText();
      animation->boneCount = atoi(text.c_str());

      text = findAll(shared->scaElement, "name", "numFrames")[0]->GetText();
      animation->frameCount = atoi(text.c_str());

      animDataElement = findAll(shared->scaElement, "name", "data")[0];
      text = animDataElement->GetText();
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

   SCA::SCAData* scaData = readSCAData(animation->boneCount, bytes);

   // int newFrameCount = -1;
   getFrames(animation, scaData);

   // Testing
   {
      //std::stringstream ss;

      // Positions
      //for(int b = 0; b < animation->boneCount; ++b){
      //   for(int f = 0; f < animation->frameCount; ++f){
      //      Anims::Vector pos = animation->frames[f].positions[b];
      //      ss << "                <pos ";
      //      ss << "px=\"" << std::fixed << std::setprecision(8) << pos.data[0] << "\" ";
      //      ss << "py=\"" << std::fixed << std::setprecision(8) << pos.data[1] << "\" ";
      //      ss << "pz=\"" << std::fixed << std::setprecision(8) << pos.data[2] << "\" ";
      //      ss << "pw=\"" << std::fixed << std::setprecision(8) << 0.0f << "\" ";
      //      ss << "/>\n";
      //   }
      //}
      //std::string resultText = ss.str();
      //FILE* file = fopen("my positions.txt", "wb");
      //fwrite(resultText.c_str(), 1, resultText.size(), file);
      //fclose(file);

      // Rotations
      //for(int b = 0; b < animation->boneCount; ++b){
      //   for(int f = 0; f < animation->frameCount; ++f){
      //      Anims::Quat rot = animation->frames[f].rotations[b];
      //      ss << "                <rot ";
      //      ss << "rx=\"" << std::fixed << std::setprecision(8) << rot.data[0] << "\" ";
      //      ss << "ry=\"" << std::fixed << std::setprecision(8) << rot.data[1] << "\" ";
      //      ss << "rz=\"" << std::fixed << std::setprecision(8) << rot.data[2] << "\" ";
      //      ss << "rw=\"" << std::fixed << std::setprecision(8) << rot.data[3] << "\" ";
      //      ss << "/>\n";
      //   }
      //}
      //std::string resultText = ss.str();
      //FILE* file = fopen("my rotations.txt", "wb");
      //fwrite(resultText.c_str(), 1, resultText.size(), file);
      //fclose(file);
   }

   bytes.clear();
   writeFramesAsSCA(animation, bytes);

   // Debug output
   //file = fopen("havok SCA data.re.bin", "wb");
   //fwrite(bytes.data(), 1, bytes.size(), file);
   //fclose(file);

   // Write binary data as text
   {
      char* textBuffer = new char[bytes.size() * 4];
      char* textP = textBuffer;

      for(size_t n = 0; n < bytes.size(); ++n){
         int written = sprintf(textP, "%d%s", bytes[n], n % 16 == 15 ? "\n" : " ");
         textP += written;
      }

      animDataElement->SetText(textBuffer);
      delete textBuffer;
   }

   findAll(shared->scaElement, "name", "floatBlockOffsets")[0]->SetText(bytes.size());
   animDataElement->SetAttribute("numelements", bytes.size());

   shared->xml.SaveFile(utf16ToUtf8(shared->xmlPath).c_str());

   hkxcmdXmlToHkx(shared->xmlPath.c_str(), shared->destAnimPath.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Other tools

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
