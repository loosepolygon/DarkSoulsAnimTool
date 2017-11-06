#pragma once

#include <string>
#include <vector>

typedef unsigned char byte;

struct AnimId {
   int animId;
   int offset;
};

struct AnimGroup {
   int firstAnimId;
   int lastAnimId;
   int firstAnimIdOffset;
};

struct Event {
   int type;
   float beginTime;
   float endTime;

   union U {
      struct T0MovementInfo {
         int unk1;
         int unk2;
         int unk3;
         int unk4;
      } t0;

      struct T1ParticleEffect {
         int unk1;
         int unk2;
         int unk3;
         int unk4;
      } t1;

      struct T129SoundEffect {
         int unk1;
         int unk2;
         int unk3;
         int unk4;
         int unk5;
      } t129;

      struct T224HitboxInfo {
         int unk1;
         int unk2;
      } t224;

      struct T233Unknown {
         int unk[93];
      } t233;

      // I might only use just this
      int vars[93];
   } u;

   int offsets[3];

   int size;
   bool shouldScaleDuration;
};

struct AnimFile {
   int type;
   std::wstring name;

   union U{
      struct AnimFileType0 {
         int dataOffset;
         int nameOffset;
         int unk1;
         int unk2;
         int unk3;
      } animFileType0;

      struct AnimFileType1 {
         int dataOffset;
         int nextFileOffset;
         int linkedAnimId;
         int unk1;
         int unk2;
         int unk3;
      } animFileType1;
   } u;
};

struct AnimData {
   struct Header{
      int eventCount;
      int eventOffset;
      int unk1;
      int unk2;
      int unk3;
      int someEventOffset;
      int animFileOffset;
   } header;

   std::vector<Event> events;
   AnimFile animFile;
};

struct TaeFile {
   struct Header {
      char signature[4];
      int unk1;
      int unk2;
      int fileSize;
      int unk3[16];
      int unk4;
      int animIdCount;
      int animIdsOffset;
      int animGroupsOffset;
      int unk5;
      int animDataCount;
      int animDataOffset;
      int unk6[10];
      int fileNamesOffset;
   } header;

   std::wstring skeletonHkxName;
   std::wstring sibName;

   std::vector<AnimId> animIds;
   std::vector<AnimGroup> animGroups;
   std::vector<AnimData> animData;
};
