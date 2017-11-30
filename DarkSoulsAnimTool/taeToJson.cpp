#include "structs.hpp"
#include "funcs.hpp"
#include "json.hpp"
#include "knownEventTypes.hpp"

#include <string>
#include <cstdio>
#include <vector>
#include <algorithm>

std::string getUnknown(void* source) {
   char text[64];
   int i = *reinterpret_cast<int*>(source);
   float f = *reinterpret_cast<float*>(source);
   snprintf(text, sizeof(text), "%d or %.3f", i, f);

   return text;
}

json::JSON getUnknownArray(int* source, int count) {
   json::JSON result = json::Array();

   for (int n = 0; n < count; ++n) {
      result.append(getUnknown(&source[n]));
   }

   return result;
}

// Note: keys are sorted by alphabetical order
json::JSON eventToJson(Event event) {
   json::JSON result = {
      "type", event.type,
      "beginTime", event.beginTime,
      "endTime", event.endTime,
   };

   static json::JSON knownEventTypes = json::JSON::Load(knownEventTypesText);
   json::JSON knownEventInfo = knownEventTypes[std::to_string(event.type)];

   if (!knownEventInfo.IsNull()) {
      result["typeName"] = knownEventInfo["eventTypeName"].ToString();
   }

   json::JSON jsonVars = json::Object();

   int varCount = event.size / 4 - 1;
   char varKey[64];
   for (int n = 0; n < varCount; ++n) {
      if (varCount > 10) {
         snprintf(varKey, sizeof(varKey), "var%02d", n);
      }else{
         snprintf(varKey, sizeof(varKey), "var%d", n);
      }

      if (!knownEventInfo.IsNull()) {
         json::JSON varInfo = knownEventInfo[std::to_string(n)];
         if (!varInfo.IsNull()) {
            strcat(varKey, ("_" + varInfo["varName"].ToString()).c_str());

            int* intValue = reinterpret_cast<int*>(&event.vars) + n;
            float* floatValue = (float*)intValue;

            std::string valueType = varInfo["valueType"].ToString();
            if (valueType == "int") {
               jsonVars[varKey] = *intValue;
            }else if (valueType == "float") {
               jsonVars[varKey] = *floatValue;
            }else {
               printf("Bad known event info for %d\n", event.type);
               throw new std::exception();
            }
         }else {
            jsonVars[varKey] = getUnknown(reinterpret_cast<int*>(&event.vars) + n);
         }
      }else {
         jsonVars[varKey] = getUnknown(reinterpret_cast<int*>(&event.vars) + n);
      }
   }

   result["vars"] = jsonVars;

   return result;
}

json::JSON taeToJson(TaeFile* taeFile) {
   json::JSON root;

   auto header = taeFile->header;

   root["header"] = {
      "unk1", getUnknown(&header.unk1),
      "unk2", getUnknown(&header.unk2),
      "unk3", getUnknownArray(header.unk3, sizeof(header.unk3) / 4),
      "unk4", getUnknown(&header.unk4),
      "unk5", getUnknown(&header.unk5),
      "unk6", getUnknownArray(header.unk6, sizeof(header.unk6) / 4),
   };

   root["skeletonHkxName"] = utf16ToUtf8(taeFile->skeletonHkxName);
   root["sibName"] = utf16ToUtf8(taeFile->sibName);

   auto jsonAnimIds = json::Array();
   for (AnimId animId : taeFile->animIds) {
      jsonAnimIds.append(animId.animId);
   }
   root["animIds"] = jsonAnimIds;

   auto jsonAnimGroups = json::Array();
   for (AnimGroup animGroup : taeFile->animGroups) {
      json::JSON obj = {
         "firstAnimId", animGroup.firstAnimId,
         "lastAnimId", animGroup.lastAnimId,
      };
      jsonAnimGroups.append(obj);
   }
   root["animGroups"] = jsonAnimGroups;

   auto jsonAnimData = json::Array();
   for (AnimData animData : taeFile->animData) {
      json::JSON dataHeader = {
         "unk1", getUnknown(&animData.header.unk1),
         "unk2", getUnknown(&animData.header.unk2),
         "unk3", getUnknown(&animData.header.unk3),
      };

      // Sort events by their type number for easier analysis
      std::sort(
         animData.events.begin(),
         animData.events.end(),
         [](const Event& e1, const Event& e2) -> bool {return e1.type < e2.type;}
      );

      auto events = json::Array();
      for (Event event : animData.events) {
         events.append(eventToJson(event));
      }

      json::JSON animFile = {
         "type", animData.animFile.type,
         "name", utf16ToUtf8(animData.animFile.name),
      };
      if (animData.animFile.type == 0) {
         animFile["unk1"] = animData.animFile.u.animFileType0.unk1;
         animFile["unk2"] = animData.animFile.u.animFileType0.unk2;
         animFile["unk3"] = animData.animFile.u.animFileType0.unk3;
      }else {
         animFile["linkedAnimId"] = animData.animFile.u.animFileType1.linkedAnimId;
         animFile["unk1"] = animData.animFile.u.animFileType1.unk1;
         animFile["unk2"] = animData.animFile.u.animFileType1.unk2;
         animFile["unk3"] = animData.animFile.u.animFileType1.unk3;
      }

      json::JSON obj = {
         "header", dataHeader,
         "events", events,
         "animFile", animFile,
      };
      jsonAnimData.append(obj);
   }
   root["animData"] = jsonAnimData;

   return root;
}
