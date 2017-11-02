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
		} t0MovementInfo;

		struct T1ParticleEffect {
			int unk1;
			int unk2;
			int unk3;
			int unk4;
		} t1ParticleEffect;

		struct T16Unknown {
			int unk1;
			int unk2;
			int unk3;
			int unk4;
			int unk5;
		} t16Unknown;

		struct T96Unknown {
			int unk1;
			int unk2;
			int unk3;
			int unk4;
		} t96Unknown;

		struct T108Unknown {
			int unk1;
			int unk2;
			int unk3;
			int unk4;
		} t108Unknown;

		struct T128Unknown {
			int unk1;
			int unk2;
			int unk3;
		} t128Unknown;

		struct T129SoundEffect {
			int unk1;
			int unk2;
			int unk3;
			int unk4;
			int unk5;
		} t129SoundEffect;

		struct T144Unknown {
			int unk1;
			int unk2;
			int unk3;
			int unk4;
		} t144Unknown;

		struct T224HitboxInfo {
			int unk1;
			int unk2;
		} t224HitboxInfo;

		struct T303Unknown {
			int unk1;
			int unk2;
		} t303Unknown;
	} u;

	bool shouldScaleDuration;
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
		byte unk3[0x44];
		int animIdCount;
		int animIdsOffset;
		int animGroupsOffset;
		byte unk4[0x4];
		int animDataCount;
		int animDataOffset;
		byte unk5[0x28];
		int fileNamesOffset;
	} header;

	std::wstring skeletonHkxName;
	std::wstring sibName;

	std::vector<AnimId> animIds;
	std::vector<AnimGroup> animGroups;
	std::vector<AnimData> animData;
};

TaeFile* readTaeFile(FILE* file);

void scaleAnim(
   std::wstring sourceTaePath,
   std::wstring animFileName,
   float scale
);
