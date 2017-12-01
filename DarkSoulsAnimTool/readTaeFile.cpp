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
   int peekChar = fgetc(file);
   ungetc(peekChar, file);
   if (peekChar > 1) {
      while (feof(file) == 0) {
         wchar_t c;
         fread(&c, 1, sizeof(wchar_t), file);

         if (c == L'\0') {
            break;
         }

         text += c;
      }
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
   TaeFile* taeFile = new TaeFile{};

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
      }else {
         printf("Unknown AnimFile type: %d\n", animFile.type);
         throw new std::exception();
      }
      animData.animFile = animFile;

      if (animFile.type == 1 && animHeader.eventCount == 0) {
         std::wstring name = readNameW(file, ftell(file));
         animData.animFile.name = name;
      }else {
         fseek(file, animHeader.eventOffset, SEEK_SET);
      }

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

            // Always the next offset?
            int paramsOffset;
            fread(&paramsOffset, sizeof(int), 1, file);
            fseek(file, paramsOffset, SEEK_SET);

            event.paramCount = getEventParamCount(event.type);
            if(event.paramCount == -1){
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
            }

            event.shouldScaleDuration = true;
            // Don't scale sounds, it cuts off the sound early and sounds awful
            if (event.type == 129) {
               event.shouldScaleDuration = false;
            }

            fread(&event.params, event.paramCount * sizeof(int), 1, file);
         }
         fseek(file, posBuffer, SEEK_SET);

         animData.events.push_back(event);
      }
   }

   return taeFile;
}

int getEventParamCount(int eventType) {
   switch (eventType) {
      case 0: return 3;
      case 1: return 3;
      case 2: return 4;
      case 5: return 2;
      case 8: return 12;
      case 16: return 4;
      case 24: return 4;
      case 32: return 1;
      case 33: return 1;
      case 64: return 2;
      case 65: return 1;
      case 66: return 1;
      case 67: return 1;
      case 96: return 3;
      case 99: return 3;
      case 100: return 8;
      case 101: return 1;
      case 104: return 3;
      case 108: return 3;
      case 109: return 3;
      case 110: return 1;
      case 112: return 2;
      case 114: return 3;
      case 115: return 3;
      case 116: return 3;
      case 118: return 3;
      case 119: return 3;
      case 120: return 6;
      case 121: return 2;
      case 128: return 2;
      case 129: return 4;
      case 130: return 4;
      case 144: return 3;
      case 145: return 1;
      case 193: return 2;
      case 224: return 1;
      case 225: return 1;
      case 226: return 1;
      case 228: return 3;
      case 229: return 1;
      case 231: return 1;
      case 232: return 1;
      case 233: return 2;
      case 236: return 3;
      case 300: return 4;
      case 301: return 1;
      case 302: return 1;
      case 303: return 1;
      case 304: return 2;
      case 306: return 3;
      case 307: return 2;
      case 308: return 6;
      case 401: return 1;
      case 500: return 1;
      default: return -1;
   }
}
