#include "structs.hpp"
#include "funcs.hpp"

using namespace SCA;
using namespace DataReading;

////////////////////////////////////////////////////////////////////////////////////////////////////
// NURBS curve handling
// Adopted from https://www.codeproject.com/Articles/1095142/Generate-and-understand-NURBS-curves

float nip(int cpIndex, int cpCount, int degree, std::vector<byte> allKnotBytes, float frame) {
   const int maxOrder = 3 + 1;
   float result[maxOrder + 1];
   float knots[maxOrder + 1];

   for(int n = 0; n < degree + 2; ++n){
      knots[n] = (float)allKnotBytes[cpIndex + n] / (float)(cpCount - 1);
   }
   frame /= (float)(cpCount - 1);

   size_t m = allKnotBytes.size() - 1;
   float firstKnot = (float)allKnotBytes[0] / (float)(cpCount - 1);
   float lastKnot = (float)allKnotBytes[m] / (float)(cpCount - 1);
   if(
      (cpIndex == 0 && frame == lastKnot) ||
      (cpIndex == (m - degree - 1) && frame == lastKnot)
   ){
      return 1;
   }

   if(frame < knots[0] || frame >= knots[degree + 1]){
      return 0;
   }

   for (int j = 0; j <= degree; j++) {
      if (frame >= knots[j] && frame < knots[j + 1])
         result[j] = 1;
      else
         result[j] = 0;
   }

   float saved;
   float temp;
   for (int k = 1; k <= degree; k++) {
      if(result[0] == 0){
         saved = 0;
      }else{
         saved = ((frame - knots[0]) * result[0]) / (knots[k] - knots[0]);
      }

      for (int j = 0; j < degree - k + 1; j++) {
         float Uleft = knots[j + 1];
         float Uright = knots[j + k + 1];

         if (result[j + 1] == 0) {
            result[j] = saved;
            saved = 0;
         } else {
            temp = result[j + 1] / (Uright - Uleft);
            result[j] = saved + (Uright - frame) * temp;
            saved = (frame - Uleft) * temp;
         }
      }
   }
   return result[0];
}

template<typename T>
T getNurbsControlPoint(
   const std::vector<T>& controlPoints,
   bool* mask,
   int degree,
   const std::vector<byte>& knots,
   float frame
){
   T result;

   for(int m = 0; m < 4; ++m) if(mask[m]) result.data[m] = 0.0f;

   float rationalWeight = 0;
   for(size_t n = 0; n < controlPoints.size(); n++){
      float temp = nip(n, controlPoints.size(), degree, knots, frame);
      // temp *= 1.0f; // weight is always 1
      rationalWeight += temp;
   }

   for(size_t n = 0; n < controlPoints.size(); n++){
      float mult = nip(n, controlPoints.size(), degree, knots, frame);
      // mult *= 1.0f;
      mult /= rationalWeight;
      for(int m = 0; m < 4; ++m){
         if(mask[m]){
            result.data[m] += controlPoints[n].data[m] * mult;
         }
      }
   }

   return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Rest of the SCA stuff

int getXBitInt(byte* source, int size, int from) {
   int currentIndex = from;
   int result = 0;
   for (int n = 0; n < size; ++n) {
      int byteIndex = currentIndex / 8;
      int bitIndex = currentIndex % 8;

      int increment = (source[byteIndex] & (1 << bitIndex)) > 0 ? 1 : 0;
      increment *= (1 << n);
      result += increment;

      ++currentIndex;
   }

   return result;
}

float getQuatFloat(int number, int numBits){
   float max = (float)((1 << numBits) - 2);
   float ratio = (((float)number / max) - 0.5f) * 2.0f;
   float result = ratio * (sqrtf(2.0f) / 2.0f);

   if(number > (int)max){
      throw "getQuatFloat error";
   }

   return result;
}

Quat readQuat(DataReader& dataReader, int rotQuantizationSize){
   byte quatBytes[16];
   for(int n = 0; n < rotQuantizationSize; ++n){
      quatBytes[n] = readSingle<byte>(dataReader);
   }

   float f1, f2, f3;
   int missingIndex;
   bool isNegative;
   if(rotQuantizationSize == 5){
      f1 = getQuatFloat(getXBitInt(quatBytes, 12, 00), 12);
      f2 = getQuatFloat(getXBitInt(quatBytes, 12, 12), 12);
      f3 = getQuatFloat(getXBitInt(quatBytes, 12, 24), 12);
      missingIndex = getXBitInt(quatBytes, 2, 36);
      isNegative = (bool)getXBitInt(quatBytes, 1, 38);
   }else{
      throw "readQuat: rotQuantizationSize not implemented; throw a brick at developer";
   }

   float fM = sqrtf(1.0f - f1*f1 - f2*f2 - f3*f3);
   if(isNegative) fM = -fM;
   if(missingIndex == 0) return {fM, f1, f2, f3};
   if(missingIndex == 1) return {f1, fM, f2, f3};
   if(missingIndex == 2) return {f1, f2, fM, f3};
   if(missingIndex == 3) return {f1, f2, f3, fM};
   throw "readQuat: invalid missingIndex";
}

SCAData* readSCAData(int trackCount, const std::vector<byte>& bytes){
   SCAData* scaData = new SCAData;

   DataReader dataReader(bytes);

   // Read maskAndQuantization
   scaData->maskAndQuantization.resize(trackCount);
   for(int n = 0; n < trackCount; ++n){
      scaData->maskAndQuantization[n] = readSingle<int>(dataReader);
   }

   // Set up trackSegments from the masks
   for(int n = 0; n < trackCount; ++n){
      byte* byte4 = (byte*)(&scaData->maskAndQuantization[n]);

      int posQuantizationSize =   ((byte4[0] >> 0) & 3) ? 2 : 1;
      int quatQuantizationEnum =  ((byte4[0] >> 2) & 15);
      int scaleQuantizationSize = ((byte4[0] >> 6) & 3) ? 2 : 1;

      int rotQuantizationSize = 0;
      switch(quatQuantizationEnum){
         case 0: rotQuantizationSize = 4; break;
         case 1: rotQuantizationSize = 5; break;
         case 2: rotQuantizationSize = 6; break;
         case 3: rotQuantizationSize = 3; break;
         case 4: rotQuantizationSize = 2; break;
         case 5: rotQuantizationSize = 16; break;
         default: throw "Bad quat quantization value";
      }

      for(int b = 1; b <= 3; ++b){
         // Default new segment
         TrackSegment newSegment;
         newSegment.trackIndex = n;
         if(b == 1){
            newSegment.tType = TType::position;
            newSegment.quantizationSize = posQuantizationSize;
         }else if(b == 2){
            newSegment.tType = TType::rotation;
            newSegment.quantizationSize = rotQuantizationSize;
         }else if(b == 3){
            newSegment.tType = TType::scale;
            newSegment.quantizationSize = scaleQuantizationSize;
            // Identity scale is 1,1,1,1
            for(int m = 0; m < 4; ++m) newSegment.defaultVector.data[m] = 1.0f;
         }

         byte mask = byte4[b];
         bool staticX = mask & (1 << 0);
         bool staticY = mask & (1 << 1);
         bool staticZ = mask & (1 << 2);
         bool staticW = mask & (1 << 3);
         bool identX = (~mask & (1 << 4)) && !staticX;
         bool identY = (~mask & (1 << 5)) && !staticY;
         bool identZ = (~mask & (1 << 6)) && !staticZ;
         bool identW = (~mask & (1 << 7)) && !staticW;

         bool isChanging4[4] = {
            !staticX && !identX,
            !staticY && !identY,
            !staticZ && !identZ,
            !staticW && !identW,
         };
         int isChangingCount = 0;
         for(int m = 0; m < 4; ++m) if(isChanging4[m]) ++isChangingCount;

         bool isStatic4[4] = {
            staticX && !identX,
            staticY && !identY,
            staticZ && !identZ,
            staticW && !identW,
         };
         int isStaticCount = 0;
         for(int m = 0; m < 4; ++m) if(isStatic4[m]) ++isStaticCount;

         bool isVector = b != 2;
         bool addedChangingQuat = false;

         if(isChangingCount > 0){
            TrackSegment changingSegment = newSegment;
            changingSegment.isStatic = false;
            if(isVector){
               *(int*)(changingSegment.mask) = *(int*)(isChanging4);
            }else{
               addedChangingQuat = true;
            }

            scaData->trackSegments.push_back(changingSegment);
         }

         if(isStaticCount > 0 && addedChangingQuat == false){
            TrackSegment staticSegment = newSegment;
            staticSegment.isStatic = true;
            if(isVector){
               *(int*)(staticSegment.mask) = *(int*)(isStatic4);
            }

            scaData->trackSegments.push_back(staticSegment);
         }
      }
   }

   // Read trackSegments
   for(TrackSegment& segment : scaData->trackSegments){
      // Static vector:
      if(segment.tType != TType::rotation && segment.isStatic){
         segment.staticVector = segment.defaultVector;

         for(int m = 0; m < 4; ++m){
            if(segment.mask[m]){
               segment.staticVector.data[m] = readSingle<float>(dataReader);
            }
         }

         continue;
      }

      // Static quat:
      if(segment.tType == TType::rotation && segment.isStatic){
         segment.staticQuat = readQuat(dataReader, segment.quantizationSize);

         align(dataReader, 4);

         continue;
      }

      align(dataReader, 4);

      // NURBS data
      auto& nurbs = segment.nurbs;
      nurbs.controlPointCount = readSingle<short>(dataReader) + 1;
      nurbs.degree = readSingle<byte>(dataReader);
      nurbs.knots.resize(nurbs.controlPointCount + nurbs.degree + 1);
      readArray<byte>(dataReader, nurbs.knots.data(), nurbs.knots.size());

      // Changing vector:
      if(segment.tType != TType::rotation){
         auto& vectors = segment.vectors;

         align(dataReader, 4);

         for(int m = 0; m < 4; ++m){
            if(segment.mask[m]){
               vectors.min.data[m] = readSingle<float>(dataReader);
               vectors.max.data[m] = readSingle<float>(dataReader);
            }
         }

         auto readVector = [&](Vector& target){
            target = segment.defaultVector;

            for(int m = 0; m < 4; ++m){
               if(segment.mask[m]){
                  float& min = vectors.min.data[m];
                  float& max = vectors.max.data[m];

                  float value;
                  if(segment.quantizationSize == 8){
                     byte byteValue = readSingle<byte>(dataReader);
                     value = (float)(byteValue) / 255.0f * (max - min) + min;
                  }else{
                     uint16_t shortValue = readSingle<uint16_t>(dataReader);
                     value = (float)(shortValue) / 65535.0f * (max - min) + min;
                  }

                  target.data[m] = value;
               }
            }
         };

         vectors.controlPoints.insert(
            vectors.controlPoints.begin(),
            nurbs.controlPointCount,
            segment.defaultVector
         );
         for(int n = 0; n < nurbs.controlPointCount; ++n){
            readVector(vectors.controlPoints[n]);
         }

         continue;
      }

      // Changing quat:
      if(segment.tType == TType::rotation){
         auto& quats = segment.quats;

         quats.controlPoints.resize(nurbs.controlPointCount);
         for(int n = 0; n < nurbs.controlPointCount; ++n){
            quats.controlPoints[n] = readQuat(dataReader, segment.quantizationSize);
         }

         align(dataReader, 4);

         continue;
      }
   }

   if(dataReader.cursor != dataReader.bytes.size() - 4){
      throw "Read SCA incorrectly or there is a float track";
   }

   return scaData;
}

void getFrames(Anims::Animation* animation, SCAData* scaData){
   animation->frames.clear();

   animation->frames.resize(animation->frameCount);
   for(size_t n = 0; n < animation->frames.size(); ++n){
      Anims::Frame& frame = animation->frames[n];

      frame.number = n;

      frame.positions.resize(animation->boneCount);
      frame.rotations.resize(animation->boneCount);
      frame.scales.insert(
         frame.scales.begin(),
         animation->boneCount,
         {1.0f, 1.0f, 1.0f}
      );
   }

   for(TrackSegment& segment : scaData->trackSegments){
      // Static vector:
      if(segment.isStatic && segment.tType != TType::rotation){
         Anims::Vector staticVector;
         memcpy(staticVector.data, segment.staticVector.data, sizeof(float) * 3);

         for(Anims::Frame& frame : animation->frames){
            if(segment.tType == TType::position){
               frame.positions[segment.trackIndex] = staticVector;
            }else{
               frame.scales[segment.trackIndex] = staticVector;
            }
         }

         continue;
      }

      // Static quat:
      if(segment.isStatic && segment.tType == TType::rotation){
         Anims::Quat staticQuat;
         memcpy(staticQuat.data, segment.staticQuat.data, sizeof(float) * 4);

         for(Anims::Frame& frame : animation->frames){
            frame.rotations[segment.trackIndex] = staticQuat;
         }

         continue;
      }

      // Changing vector:
      if(!segment.isStatic && segment.tType != TType::rotation){
         // for(Anims::Frame& frame : animation->frames){
         for(int n = 0; n < animation->frames.size(); ++n){
            auto& frame = animation->frames[n];

            Vector vector = getNurbsControlPoint<Vector>(
               segment.vectors.controlPoints,
               segment.mask,
               segment.nurbs.degree,
               segment.nurbs.knots,
               (float)frame.number
            );
            if(segment.tType == TType::position){
               memcpy(frame.positions[segment.trackIndex].data, vector.data, sizeof(float) * 3);
            }else{
               memcpy(frame.scales[segment.trackIndex].data, vector.data, sizeof(float) * 3);
            }
         }
      }

      // Changing quat:
      if(!segment.isStatic && segment.tType == TType::rotation){
         for(Anims::Frame& frame : animation->frames){
            Quat quat = getNurbsControlPoint<Quat>(
               segment.quats.controlPoints,
               segment.mask,
               segment.nurbs.degree,
               segment.nurbs.knots,
               (float)frame.number
            );
            memcpy(frame.rotations[segment.trackIndex].data, quat.data, sizeof(float) * 4);
         }
      }
   }
}
