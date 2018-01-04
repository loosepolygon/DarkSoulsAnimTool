
namespace DataReading{
   inline void checkValid(DataReader& reader, int readSize){
      int bytesLeft = reader.bytes.size() - reader.cursor;
      if(readSize > bytesLeft){
         throw "DataReader: tried to read past end of data";
      }
   }

   template<typename T>
   T readSingle(DataReader& reader){
      checkValid(reader, sizeof(T));

      T* pointer = (T*)(&reader.bytes[reader.cursor]);
      reader.cursor += sizeof(T);
      return *pointer;
   }

   template<typename T>
   void readArray(DataReader& reader, T* dest, int count){
      checkValid(reader, sizeof(T) * count);

      T* source = (T*)(&reader.bytes[reader.cursor]);
      for(int n = 0; n < count; ++n){
         dest[n] = source[n];

         reader.cursor += sizeof(T);
      }
   }

   inline void align(DataReader& reader, int num){
      int diff = (reader.cursor % num);
      if(diff != 0){
         reader.cursor += num - diff;
      }
   }
}
