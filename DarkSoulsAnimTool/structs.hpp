#pragma once

#include <string>
#include <vector>

typedef unsigned char byte;

namespace DataReading{
   struct DataReader{
      const std::vector<byte>& bytes;
      int cursor = 0;

      DataReader(const std::vector<byte>& bytes2) :
         bytes(bytes2)
      {}
   };
}

namespace TAE{
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

      //int paramsOffset;
      int paramCount;
      int params[12];

      int offsets[3];
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
         int timeFloatCount;
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
}

namespace SCA{
   struct Vector{
      float data[4] = {0};
   };

   struct Quat{
      float data[4] = {0};
   };

   // A "segment" could be an xz vector
   // (with another "segment" of a static Y float after)
   struct TrackSegment{
      int trackIndex = -1;
      // Only used for vector components
      bool mask[4] = {false};
      Vector defaultVector;
      bool isStatic = false;
      bool isVector = false;
      int quantizationSize = 0;

      Vector staticVector;

      Quat staticQuat;

      struct{
         short controlPointCount = 0;
         short degree = 0;
         std::vector<byte> knots;
      } nurbs;

      struct{
         Vector min;
         Vector max;
         std::vector<Vector> controlPoints;
         // TODO: Is this still here for anims that don't loop?
         Vector copyOfFirst;
      } vectors;

      struct{
         std::vector<Quat> controlPoints;
         Quat copyOfFirst;
      } quats;
   };

   struct SCAData{
      std::vector<int> maskAndQuantization;
      std::vector<TrackSegment> trackSegments;
   };
}

// TODO: lol
using namespace TAE;
