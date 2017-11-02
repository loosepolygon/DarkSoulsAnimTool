#include "stdafx.h"

#include "structs.hpp"
#include "utility.hpp"
#include "json.hpp"

#include <string>
#include <cstdio>
#include <vector>
#include <algorithm>

void scaleAnim(
   std::wstring sourceTaePath,
   std::wstring animFileName,
   float scale
) {
	FILE* file = _wfopen(sourceTaePath.c_str(), L"rb+");

	if (file == NULL) {
      wprintf_s(L"Cannot open file: %s\n", sourceTaePath.c_str());
		return;
	}

	TaeFile* taeFile = readTaeFile(file);

	byte* bytes = NULL;
	int fileSize = 0;
	std::vector<Event>* events = NULL;

	std::transform(animFileName.begin(), animFileName.end(), animFileName.begin(), towlower);

	bool found = false;
	for (size_t n = 0; n < taeFile->animData.size(); ++n) {
		std::wstring nameLowercase = taeFile->animData[n].animFile.name;
		std::transform(nameLowercase.begin(), nameLowercase.end(), nameLowercase.begin(), towlower);

		std::wstring nameWithoutWin = nameLowercase;
		if (nameWithoutWin.size() > 3) {
			nameWithoutWin.resize(nameWithoutWin.size() - 3);
		}

		if (nameLowercase == animFileName || nameWithoutWin == animFileName) {
			found = true;

			events = &taeFile->animData[n].events;

			fileSize = taeFile->header.fileSize;
			bytes = new byte[fileSize];
			fseek(file, 0, SEEK_SET);
			fread(bytes, 1, fileSize, file);

			break;
		}
	}

	if (found) {
		std::wstring destTaePath = sourceTaePath + L".out";

		wprintf_s(L"Found anim file, saving to %s...\n", destTaePath.c_str());

		std::vector<int> offsetsChanged;

		for (size_t e = 0; e < events->size(); ++e) {
			Event& event = (*events)[e];

			void* ptr = &bytes[event.offsets[0]];
			float& startTimeRef = *reinterpret_cast<float*>(ptr);
			ptr = &bytes[event.offsets[1]];
			float& endTimeRef = *reinterpret_cast<float*>(ptr);
			float duration = endTimeRef - startTimeRef;

			int offset = event.offsets[0];
			bool found = std::find(offsetsChanged.begin(), offsetsChanged.end(), offset) != offsetsChanged.end();
			if (found == false) {
				startTimeRef *= scale;
				offsetsChanged.push_back(offset);
			}

			offset = event.offsets[1];
			found = std::find(offsetsChanged.begin(), offsetsChanged.end(), offset) != offsetsChanged.end();
			if (found == false) {
				if (event.shouldScaleDuration) {
					endTimeRef *= scale;
				}else{
					endTimeRef = startTimeRef + duration;
				}

				offsetsChanged.push_back(offset);
			}
		}

		_wfreopen(destTaePath.c_str(), L"wb", file);

		fwrite(bytes, 1, fileSize, file);

		printf("Done!\n");
	}else{
		wprintf_s(L"Did not find %s in %s \n", animFileName.c_str(), sourceTaePath.c_str());
	}

	fclose(file);
}

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

json::JSON eventToJson(Event event) {
   json::JSON result = {
      "type", event.type,
      "startTime", event.startTime,
      "endTime", event.endTime,
   };

   for (int n = 0; n < sizeof(Event::U) / 4; ++n) {
      char unkKey[16];
      snprintf(unkKey, sizeof(unkKey), "unk%d", n + 1);
      result[unkKey] = getUnknown(reinterpret_cast<int*>(&event.u) + n);
   }

   return result;
}

void exportTae(std::wstring sourceTaePath) {
   FILE* file = _wfopen(sourceTaePath.c_str(), L"rb");

   if (file == NULL) {
      wprintf_s(L"Cannot open file: %s\n", sourceTaePath.c_str());
      return;
   }

   TaeFile* taeFile = readTaeFile(file);

   json::JSON root;

   auto header = taeFile->header;

   std::vector<int> blep;
   blep.push_back(111);
   blep.push_back(222);

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

      }else {
         animFile["linkedAnimId"] = animData.animFile.u.animFileType1.linkedAnimId;
      }

      json::JSON obj = {
         "header", dataHeader,
         "events", events,
         "animFile", animFile,
      };
      jsonAnimData.append(obj);
   }
   root["animData"] = jsonAnimData;

   std::string jsonText = root.dump();

   std::wstring jsonOutputPath = sourceTaePath + L".json";
   wprintf_s(L"Exporting %s to %s...\n", sourceTaePath.c_str(), jsonOutputPath.c_str());
   file = _wfopen(jsonOutputPath.c_str(), L"wb");

   fwrite(jsonText.c_str(), 1, jsonText.size(), file);

   fclose(file);
}
