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
   float startTime;
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

      struct T1Unknown {
         int unk[13];
      } t8;

      struct T16Unknown {
         int unk1;
         int unk2;
         int unk3;
         int unk4;
         int unk5;
      } t16;

      struct T96Unknown {
         int unk1;
         int unk2;
         int unk3;
         int unk4;
      } t96;

      struct T108Unknown {
         int unk1;
         int unk2;
         int unk3;
         int unk4;
      } t108;

      struct T128Unknown {
         int unk1;
         int unk2;
         int unk3;
      } t128;

      struct T129SoundEffect {
         int unk1;
         int unk2;
         int unk3;
         int unk4;
         int unk5;
      } t129;

      struct T144Unknown {
         int unk1;
         int unk2;
         int unk3;
         int unk4;
      } t144;

      struct T224HitboxInfo {
         int unk1;
         int unk2;
      } t224;

      struct T228Unknown{
         int unk[30];
      } t228;

      struct T233Unknown {
         int unk[93];
      } t233;

      struct T303Unknown {
         int unk1;
         int unk2;
      } t303;
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
         int null;
      } animFileType0;

      struct AnimFileType1 {
         int dataOffset;
         int nextFileOffset;
         int linkedAnimId;
         int null1;
         int null2;
         int null3;
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
