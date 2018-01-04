#include "structs.hpp"
#include "funcs.hpp"

#include <string>
#include <cstdio>
#include <vector>
#include <exception>
#include <algorithm>

using namespace TAE;

template <typename T>
void appendData(std::vector<byte>& bytes, T* data) {
   size_t oldSize = bytes.size();
   size_t addedSize = sizeof(T);
   bytes.resize(oldSize + addedSize);
   memcpy(&bytes[oldSize], data, addedSize);
}

void appendData(std::vector<byte>& bytes, void* data, size_t addedSize) {
   size_t oldSize = bytes.size();
   bytes.resize(oldSize + addedSize);
   memcpy(&bytes[oldSize], data, addedSize);
}

void appendZeroes(std::vector<byte>& bytes, int zeroCount) {
   size_t oldSize = bytes.size();
   size_t addedSize = zeroCount;
   bytes.resize(oldSize + addedSize);
   memset(&bytes[oldSize], 0, addedSize);
}

void appendRefSpace(std::vector<byte>& bytes, int addedSize) {
   size_t oldSize = bytes.size();
   bytes.resize(oldSize + addedSize);
   memset(&bytes[oldSize], 0xEE, addedSize);
}

void alignBytes(std::vector<byte>& bytes) {
   int mod = bytes.size() % 4;
   if (mod != 0) {
      appendZeroes(bytes, 4 - mod);
   }
}

void appendNameW(std::vector<byte>& bytes, const std::wstring& name, int ref = -1) {
   size_t oldSize = bytes.size();

   if (name.size() > 0) {
      size_t addedSize = (name.size()) * 2;
      bytes.resize(oldSize + addedSize);
      memcpy(&bytes[oldSize], name.data(), addedSize);

      appendZeroes(bytes, 2);

      alignBytes(bytes);
   }

   if (ref != -1) {
      memcpy(&bytes[ref], &oldSize, sizeof(int));
   }
}

std::vector<float> getUniqueFloats(AnimData& animData) {
   std::vector<float> result;

   for (Event& event : animData.events) {
      result.push_back(event.beginTime);
      result.push_back(event.endTime);
   }

   std::sort(result.begin(), result.end());

   std::vector<float>::iterator it = std::unique(result.begin(), result.end());
   result.resize(std::distance(result.begin(), it));

   return result;
}

void writeTaeFile(std::wstring outputPath, TaeFile* taeFile) {
   // Populate members such as counts before starting to write to file

   const char sig[] = "TAE ";
   memcpy(taeFile->header.signature, sig, 4);

   taeFile->header.animIdCount = taeFile->animIds.size();
   taeFile->header.animDataCount = taeFile->animData.size();

   for (size_t n = 0; n < taeFile->animData.size(); ++n) {
      taeFile->animData[n].header.eventCount = taeFile->animData[n].events.size();
   }

   // Write file

   std::vector<byte> bytes;

   // Header
   {
      appendData(bytes, &taeFile->header);

      taeFile->header.fileNamesOffset = bytes.size();

      int refTwoNames = bytes.size();
      appendRefSpace(bytes, 8);

      appendZeroes(bytes, 8);

      appendNameW(bytes, taeFile->skeletonHkxName, refTwoNames);
      appendNameW(bytes, taeFile->sibName, refTwoNames + 4);
   }

   // Anim Ids
   {
      taeFile->header.animIdsOffset = bytes.size();

      void* source = taeFile->animIds.data();
      appendData(bytes, source, taeFile->animIds.size() * sizeof(AnimId));
   }

   // Anim groups
   {
      taeFile->header.animGroupsOffset = bytes.size();

      int groupsSize = taeFile->animGroups.size();
      appendData(bytes, &groupsSize);
      int groupsDataOffset = bytes.size() + 4;
      appendData(bytes, &groupsDataOffset);

      for (size_t n = 0; n < taeFile->animGroups.size(); ++n) {
         size_t idIndex;
         int firstAnimId = taeFile->animGroups[n].firstAnimId;
         for (idIndex = 0; idIndex < taeFile->animIds.size(); ++idIndex) {
            if (taeFile->animIds[idIndex].animId == firstAnimId) {
               int offset = taeFile->header.animIdsOffset + idIndex * sizeof(AnimId);
               taeFile->animGroups[n].firstAnimIdOffset = offset;

               break;
            }
         }

         appendData(bytes, &taeFile->animGroups[n]);
      }
   }

   // Anim data
   {
      taeFile->header.animDataOffset = bytes.size();

      for (size_t n = 0; n < taeFile->animData.size(); ++n) {
         taeFile->animIds[n].offset = bytes.size();
         appendData(bytes, &taeFile->animData[n].header);
      }

      for (size_t n = 0; n < taeFile->animData.size(); ++n) {
         AnimData& animData = taeFile->animData[n];
         AnimFile& animFile = animData.animFile;

         animData.header.animFileOffset = bytes.size();

         // AnimFile

         appendData(bytes, &animFile.type);
         if (animFile.type == 0) {
            animFile.u.animFileType0.dataOffset = bytes.size() + 4;
            appendData(bytes, &animFile.u.animFileType0);
            appendNameW(bytes, animFile.name, animFile.u.animFileType0.dataOffset);
         }else{
            animFile.u.animFileType1.dataOffset = bytes.size() + 4;
            appendData(bytes, &animFile.u.animFileType1);

            int* nfoPtr = (int*)&bytes[animFile.u.animFileType1.dataOffset];
            *nfoPtr = bytes.size();
         }

         // Events

         if (animData.events.size() > 0) {
            animData.header.someEventOffset = bytes.size();
         }else if(animFile.type == 1){
            // This is only for the end of some type1s or something
            appendNameW(bytes, animFile.name);
         }

         std::vector<float> timeFloats = getUniqueFloats(animData);
         animData.header.timeFloatCount = timeFloats.size();

         if (animData.events.size() > 0) {
            appendData(bytes, timeFloats.data(), timeFloats.size() * sizeof(float));
            animData.header.eventOffset = bytes.size();
         }

         for (size_t e = 0; e < animData.events.size(); ++e) {
            Event& event = animData.events[e];

            auto it = std::find(timeFloats.begin(), timeFloats.end(), event.beginTime);
            int beginIndex = std::distance(timeFloats.begin(), it);
            beginIndex = animData.header.someEventOffset + beginIndex * sizeof(float);

            it = std::find(timeFloats.begin(), timeFloats.end(), event.endTime);
            int endIndex = std::distance(timeFloats.begin(), it);
            endIndex = animData.header.someEventOffset + endIndex * sizeof(float);

            appendData(bytes, &beginIndex);
            appendData(bytes, &endIndex);
            appendRefSpace(bytes, 4);
         }

         for (size_t e = 0; e < animData.events.size(); ++e) {
            Event& event = animData.events[e];

            int offset3 = animData.header.eventOffset;
            offset3 += e * sizeof(float) * 3;
            offset3 += sizeof(float) * 2;
            int* typePointer = (int*)&bytes[offset3];
            *typePointer = bytes.size();

            appendData(bytes, &event.type);
            int paramsOffset = bytes.size() + 4;
            appendData(bytes, &paramsOffset);
            int paramCount = getEventParamCount(event.type);
            appendData(bytes, event.params, paramCount * sizeof(int));
         }

         // Update header
         int writtenHeaderIndex = taeFile->header.animDataOffset;
         writtenHeaderIndex += n * sizeof(AnimData::Header);
         auto writtenHeader = (AnimData::Header*)&bytes[writtenHeaderIndex];
         *writtenHeader = animData.header;
      }
   }

   // Update offsets

   auto writtenHeader = (TaeFile::Header*)&bytes[0];
   taeFile->header.fileSize = bytes.size();
   *writtenHeader = taeFile->header;

   auto writtenAnimIds = (AnimId*)&bytes[taeFile->header.animIdsOffset];
   memcpy(writtenAnimIds, taeFile->animIds.data(), taeFile->animIds.size() * sizeof(AnimId));
   
   // Write tae file

   wprintf_s(L"Saving TAE file as %s...\n", outputPath.c_str());
   FILE* file = _wfopen(outputPath.c_str(), L"wb");

   if (file == NULL) {
      wprintf_s(L"Cannot open file for writing: %s\n", outputPath.c_str());
      return;
   }

   fwrite(bytes.data(), 1, bytes.size(), file);

   fclose(file);
}
