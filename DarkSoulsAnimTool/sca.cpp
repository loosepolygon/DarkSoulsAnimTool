#include "structs.hpp"
#include "funcs.hpp"

using namespace SCA;
using namespace DataReading;

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
   float ratio = ((number / max) - 0.5f) * 2.0f;
   float result = ratio * (sqrtf(2.0f) / 2.0f);
   return result;
}

Quat readQuat(DataReader& dataReader, int quatQuantizationSize){
   byte quatBytes[16];
   for(int n = 0; n < quatQuantizationSize; ++n){
      quatBytes[n] = readSingle<byte>(dataReader);
   }

   float f1, f2, f3;
   int missingIndex;
   bool isNegative;
   if(quatQuantizationSize == 5){
      f1 = getQuatFloat(getXBitInt(quatBytes, 12, 00), 12);
      f2 = getQuatFloat(getXBitInt(quatBytes, 12, 12), 12);
      f3 = getQuatFloat(getXBitInt(quatBytes, 12, 24), 12);
      missingIndex = getXBitInt(quatBytes, 2, 36);
      isNegative = (bool)getXBitInt(quatBytes, 1, 38);
   }else{
      throw "readQuat: quatQuantizationSize not implemented; throw a brick at developer";
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

      int quatQuantizationSize = 0;
      switch(quatQuantizationEnum){
         case 0: quatQuantizationSize = 4; break;
         case 1: quatQuantizationSize = 5; break;
         case 2: quatQuantizationSize = 6; break;
         case 3: quatQuantizationSize = 3; break;
         case 4: quatQuantizationSize = 2; break;
         case 5: quatQuantizationSize = 16; break;
         default: throw "Bad quat quantization value";
      }

      for(int b = 1; b < 4; ++b){
         // Default new segment
         TrackSegment newSegment;
         newSegment.trackIndex = n;
         if(b == 3){
            // Identity scale is 1,1,1,1
            for(int m = 0; m < 4; ++m) newSegment.defaultVector.data[m] = 1.0f;
         }
         if(b == 1) newSegment.quantizationSize = posQuantizationSize;
         if(b == 2) newSegment.quantizationSize = quatQuantizationSize;
         if(b == 3) newSegment.quantizationSize = scaleQuantizationSize;

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
            changingSegment.isVector = isVector;
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
            staticSegment.isVector = isVector;
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
      if(segment.isVector && segment.isStatic){
         segment.staticVector = segment.defaultVector;

         for(int m = 0; m < 4; ++m){
            if(segment.mask[m]){
               segment.staticVector.data[m] = readSingle<float>(dataReader);
            }
         }

         continue;
      }

      // Static quat:
      if(!segment.isVector && segment.isStatic){
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
      if(segment.isVector){
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
                     short shortValue = readSingle<short>(dataReader);
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
      if(!segment.isVector){
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