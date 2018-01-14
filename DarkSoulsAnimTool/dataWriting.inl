
namespace DataWriting{
   template <typename T>
   void appendData(std::vector<byte>& bytes, T* data) {
      size_t oldSize = bytes.size();
      size_t addedSize = sizeof(T);
      bytes.resize(oldSize + addedSize);
      memcpy(&bytes[oldSize], data, addedSize);
   }

   inline void appendData(std::vector<byte>& bytes, void* data, size_t addedSize) {
      size_t oldSize = bytes.size();
      bytes.resize(oldSize + addedSize);
      memcpy(&bytes[oldSize], data, addedSize);
   }

   template <typename T>
   void appendValue(std::vector<byte>& bytes, const T& data) {
      appendData(bytes, &data);
   }

   inline void appendZeroes(std::vector<byte>& bytes, int zeroCount) {
      size_t oldSize = bytes.size();
      size_t addedSize = zeroCount;
      bytes.resize(oldSize + addedSize);
      memset(&bytes[oldSize], 0, addedSize);
   }

   inline void appendRefSpace(std::vector<byte>& bytes, int addedSize) {
      size_t oldSize = bytes.size();
      bytes.resize(oldSize + addedSize);
      memset(&bytes[oldSize], 0xEE, addedSize);
   }

   inline void alignBytes(std::vector<byte>& bytes, int alignment = 4) {
      int mod = bytes.size() % alignment;
      if (mod != 0) {
         appendZeroes(bytes, alignment - mod);
      }
   }
}
