#include "structs.hpp"
#include "funcs.hpp"
#include "json.hpp"

#include <string>
#include <cstdio>
#include <vector>
#include <algorithm>

// For debugging unknowns that are written as "123 or 0.000"
int getUnknown(json::JSON value) {
   if (value.JSONType() == json::JSON::Class::Integral) {
      return value.ToInt();
   }

   std::string text = value.ToString();
   for (size_t n = 0; n < text.size(); ++n) {
      if (text[n] == ' ') {
         text.resize(n);
         break;
      }
   }

   int result = atoi(text.c_str());

   return result;
}

void getUnknownArray(json::JSON array, int* source, size_t size) {
   for (size_t n = 0; n < size; ++n) {
      source[n] = getUnknown(array[n]);
   }
}

Event jsonToEvent(json::JSON jsonEvent) {
   Event event;

   event.beginTime = (float)jsonEvent["beginTime"].ToFloat();
   event.endTime = (float)jsonEvent["endTime"].ToFloat();
   event.type = jsonEvent["type"].ToInt();

   int varCount = jsonEvent.size() - 3;
   int readCount = -3;
   // The object range is iterated alphabetically
   for (auto it : jsonEvent.ObjectRange()) {
      if (readCount >= varCount) {
         break;
      }

      if (readCount >= 0) {
         std::string key = it.first;
         json::JSON value = it.second;
         event.u.vars[readCount] = getUnknown(value);
      }

      ++readCount;
   }

   return event;
}

TaeFile* jsonToTae(json::JSON root) {
   TaeFile* taeFile = new TaeFile;
   memset(taeFile, 0, sizeof(TaeFile));

   TaeFile::Header header = taeFile->header;
   json::JSON jsonHeader = root["header"];
   header.unk1 = getUnknown(jsonHeader["unk1"]);
   header.unk2 = getUnknown(jsonHeader["unk2"]);
   getUnknownArray(jsonHeader["unk3"], header.unk3, sizeof(header.unk3) / sizeof(int));
   header.unk4 = getUnknown(jsonHeader["unk4"]);
   header.unk5 = getUnknown(jsonHeader["unk5"]);
   getUnknownArray(jsonHeader["unk6"], header.unk6, sizeof(header.unk6) / sizeof(int));
   taeFile->header = header;

   taeFile->sibName = utf8ToUtf16(root["sibName"].ToString());
   taeFile->skeletonHkxName = utf8ToUtf16(root["skeletonHkxName"].ToString());

   for (json::JSON j : root["animIds"].ArrayRange()) {
      AnimId animId = {};
      animId.animId = j.ToInt();
      taeFile->animIds.push_back(animId);
   }

   for (json::JSON j : root["animGroups"].ArrayRange()) {
      AnimGroup animGroup = {};
      animGroup.firstAnimId = j["firstAnimId"].ToInt();
      animGroup.lastAnimId = j["lastAnimId"].ToInt();
      taeFile->animGroups.push_back(animGroup);
   }

   for (json::JSON j : root["animData"].ArrayRange()) {
      AnimData animData = {};

      animData.header.unk1 = getUnknown(j["header"]["unk1"]);
      animData.header.unk2 = getUnknown(j["header"]["unk2"]);
      animData.header.unk3 = getUnknown(j["header"]["unk3"]);

      for (json::JSON jsonEvent : j["events"].ArrayRange()) {
         animData.events.push_back(jsonToEvent(jsonEvent));
      }

      json::JSON jsonAnimFile = j["animFile"];
      animData.animFile.type = jsonAnimFile["type"].ToInt();
      animData.animFile.name = utf8ToUtf16(jsonAnimFile["name"].ToString());
      if (animData.animFile.type == 0) {
         animData.animFile.u.animFileType0.unk1 = getUnknown(jsonAnimFile["unk1"]);
         animData.animFile.u.animFileType0.unk2 = getUnknown(jsonAnimFile["unk2"]);
         animData.animFile.u.animFileType0.unk3 = getUnknown(jsonAnimFile["unk3"]);
      }else {
         animData.animFile.u.animFileType1.linkedAnimId = jsonAnimFile["linkedAnimId"].ToInt();
         animData.animFile.u.animFileType1.unk1 = getUnknown(jsonAnimFile["unk1"]);
         animData.animFile.u.animFileType1.unk2 = getUnknown(jsonAnimFile["unk2"]);
         animData.animFile.u.animFileType1.unk3 = getUnknown(jsonAnimFile["unk3"]);
      }

      taeFile->animData.push_back(animData);
   }

   // Note: sizes and offset vars will be set when the file is written to

   return taeFile;
}
