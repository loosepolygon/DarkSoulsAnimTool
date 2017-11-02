#include "stdafx.h"

#include "structs.hpp"

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
