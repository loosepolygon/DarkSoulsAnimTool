#pragma once

#include <string>
#include <vector>

typedef unsigned char byte;

enum class TType{
   invalid,
   position,
   rotation,
   scale,
};

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
      float data[4] = {0.0f, 0.0f, 0.0f, 1.0f};
   };

   // TODO: a Scale struct? Would make default identity values easier.

   // A "segment" could be an xz vector
   // (with another "segment" of a static Y float after)
   struct TrackSegment{
      int trackIndex = -1;
      // Only used for vector components
      bool mask[4] = {true, true, true, true};
      Vector defaultVector;
      bool isStatic = false;
      TType tType;
      int quantizationSize = 0;

      Vector staticVector;

      Quat staticQuat;

      struct{
         short controlPointCount = 0;
         byte degree = 0;
         // Each knot is 0 to 1
         std::vector<float> knots;
      } nurbs;

      struct{
         Vector min;
         Vector max;
         std::vector<Vector> controlPoints;
      } vectors;

      struct{
         std::vector<Quat> controlPoints;
      } quats;
   };

   struct SCAData{
      std::vector<int> maskAndQuantization;
      std::vector<TrackSegment> trackSegments;
   };
}

namespace Anims{
   struct Vector{
      float data[3] = {0};
   };

   struct Quat{
      float data[4] = {0.0f, 0.0f, 0.0f, 1.0f};
   };

   struct Frame{
      int number = -1;
      float normalizedTime = 0.0f;
      std::vector<Vector> positions;
      std::vector<Quat> rotations;
      std::vector<Vector> scales;
   };

   struct Animation{
      int boneCount = 0;
      int frameCount = 0;
      // float duration = 0.0f;
      std::vector<Frame> frames;
   };
}

using TAE::TaeFile;
