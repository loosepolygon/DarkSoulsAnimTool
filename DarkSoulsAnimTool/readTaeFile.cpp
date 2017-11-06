#include "structs.hpp"
#include "funcs.hpp"

#include <string>
#include <cstdio>
#include <vector>
#include <exception>

std::wstring readNameW(FILE* file, int offset) {
   long posBuffer = ftell(file);

   fseek(file, offset, SEEK_SET);

   std::wstring text;
   while (feof(file) == 0) {
      wchar_t c;
      fread(&c, 1, sizeof(wchar_t), file);

      if (c == L'\0') {
         break;
      }

      text += c;
   }

   fseek(file, posBuffer, SEEK_SET);

   return text;
}

std::wstring readNameW(FILE* file) {
   int offset;
   fread(&offset, 1, sizeof(int), file);

   return readNameW(file, offset);
}

TaeFile* readTaeFile(std::wstring sourceTaePath) {
   FILE* file = _wfopen(sourceTaePath.c_str(), L"rb");
   if (file == NULL) {
      wprintf_s(L"Cannot open file: %s\n", sourceTaePath.c_str());
      return nullptr;
   }

   TaeFile* taeFile = readTaeFile(file);

   fclose(file);

   return taeFile;
}

// Special thanks to Nyxojaele's 010 templates for making this easy for me.
TaeFile* readTaeFile(FILE* file) {
   TaeFile* taeFile = new TaeFile;

   fread(&taeFile->header, 1, sizeof(TaeFile::Header), file);

   taeFile->skeletonHkxName = readNameW(file);
   taeFile->sibName = readNameW(file);

   fseek(file, taeFile->header.animIdsOffset, SEEK_SET);
   taeFile->animIds.resize(taeFile->header.animIdCount);
   fread(taeFile->animIds.data(), sizeof(AnimId), taeFile->header.animIdCount, file);

   fseek(file, taeFile->header.animGroupsOffset, SEEK_SET);
   int animGroupsCount;
   int animGroupsDataOffset;
   fread(&animGroupsCount, sizeof(int), 1, file);
   fread(&animGroupsDataOffset, sizeof(int), 1, file);
   taeFile->animGroups.resize(animGroupsCount);
   fread(taeFile->animGroups.data(), sizeof(AnimGroup), animGroupsCount, file);

   fseek(file, taeFile->header.animDataOffset, SEEK_SET);
   taeFile->animData.resize(taeFile->header.animDataCount);
   for (size_t n = 0; n < taeFile->animData.size(); ++n) {
      fread(&taeFile->animData[n].header, sizeof(AnimData::Header), 1, file);
   }
   
   for (size_t n = 0; n < taeFile->animData.size(); ++n) {
      AnimData& animData = taeFile->animData[n];
      AnimData::Header animHeader = animData.header;

      if (animHeader.eventCount == 0) {
         continue;
      }

      fseek(file, animHeader.eventOffset, SEEK_SET);

      animData.events = std::vector<Event>();
      for(int e = 0; e < animHeader.eventCount; ++e){
         Event event;

         fread(event.offsets, sizeof(event.offsets), 1, file);

         long posBuffer = ftell(file);
         {
            fseek(file, event.offsets[0], SEEK_SET);
            fread(&event.beginTime, sizeof(float), 1, file);
            fseek(file, event.offsets[1], SEEK_SET);
            fread(&event.endTime, sizeof(float), 1, file);
            fseek(file, event.offsets[2], SEEK_SET);
            fread(&event.type, sizeof(int), 1, file);

            int eventSize;
            switch (event.type){
            case 0: eventSize = 20; break;
            case 1: eventSize = 20; break;
            case 2: eventSize = 24; break;
            case 5: eventSize = 16; break;
            case 8: eventSize = 56; break;
            case 16: eventSize = 24; break;
            case 24: eventSize = 24; break;
            case 32: eventSize = 12; break;
            case 33: eventSize = 12; break;
            case 64: eventSize = 16; break;
            case 65: eventSize = 12; break;
            case 66: eventSize = 12; break;
            case 67: eventSize = 12; break;
            case 96: eventSize = 20; break;
            case 99: eventSize = 20; break;
            case 100: eventSize = 40; break;
            case 101: eventSize = 12; break;
            case 104: eventSize = 20; break;
            case 108: eventSize = 20; break;
            case 109: eventSize = 20; break;
            case 110: eventSize = 12; break;
            case 112: eventSize = 16; break;
            case 114: eventSize = 20; break;
            case 115: eventSize = 20; break;
            case 116: eventSize = 20; break;
            case 118: eventSize = 20; break;
            case 119: eventSize = 20; break;
            case 120: eventSize = 32; break;
            case 121: eventSize = 16; break;
            case 128: eventSize = 16; break;
            case 129: eventSize = 24; break;
            case 130: eventSize = 24; break;
            case 144: eventSize = 20; break;
            case 145: eventSize = 12; break;
            case 193: eventSize = 16; break;
            case 224: eventSize = 12; break;
            case 225: eventSize = 12; break;
            case 226: eventSize = 12; break;
            case 228: eventSize = 124; break;
            case 229: eventSize = 12; break;
            case 231: eventSize = 12; break;
            case 232: eventSize = 12; break;
            case 233: eventSize = 376; break;
            case 236: eventSize = 20; break;
            case 300: eventSize = 24; break;
            case 301: eventSize = 12; break;
            case 302: eventSize = 12; break;
            case 303: eventSize = 12; break;
            case 304: eventSize = 16; break;
            case 306: eventSize = 20; break;
            case 307: eventSize = 16; break;
            case 308: eventSize = 32; break;
            case 401: eventSize = 12; break;
            case 500: eventSize = 12; break;
            default:
               printf("Unknown event type: %d \n", event.type);

               printf("Anim %d  Event: %d  offset: %x \n", n, e, event.offsets[2]);

               if (e < animHeader.eventCount - 1) {
                  fseek(file, posBuffer, SEEK_SET);
                  Event nextEvent;
                  fread(nextEvent.offsets, sizeof(nextEvent.offsets), 1, file);

                  int diff = nextEvent.offsets[2] - event.offsets[2];

                  printf("Size guess: %d\n\n", diff);
               }

               throw new std::exception();
               //goto end;
            }

            event.size = eventSize;

            event.shouldScaleDuration = true;
            // Don't scale sounds, it cuts off the sound early and sounds awful
            if (event.type == 129) {
               event.shouldScaleDuration = false;
            }

            // Subtract size of type, already read.
            eventSize -= sizeof(int);
            
            fread(&event.u, eventSize, 1, file);

//end:;
         }
         fseek(file, posBuffer, SEEK_SET);

         animData.events.push_back(event);
      }

      AnimFile animFile;
      fseek(file, animHeader.animFileOffset, SEEK_SET);
      fread(&animFile.type, sizeof(int), 1, file);
      if (animFile.type == 0) {
         fread(&animFile.u.animFileType0, sizeof(AnimFile::U::AnimFileType0), 1, file);

         std::wstring name = readNameW(file, animFile.u.animFileType0.nameOffset);
         animFile.name = name;
      }
      else if (animFile.type == 1) {
         fread(&animFile.u.animFileType1, sizeof(AnimFile::U::AnimFileType1), 1, file);
      }
      else {
         printf("Unknown type: %d\n", animFile.type);
         throw new std::exception();
      }
      animData.animFile = animFile;
   }

   return taeFile;
}
