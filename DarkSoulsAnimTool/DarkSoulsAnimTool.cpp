
#include "stdafx.h"

#include <string>
#include <iostream>
#include <cstdio>
#include <vector>
#include <exception>
#include <algorithm>

typedef unsigned char byte;

void HK_CALL havokErrorReport(const char* msg, void* userContext){
	printf("%s\n", msg);
}

hkResource* hkSerializeUtilLoad(hkStreamReader* stream
	, hkSerializeUtil::ErrorDetails* detailsOut/*=HK_NULL*/
	, const hkClassNameRegistry* classReg/*=HK_NULL*/
	, hkSerializeUtil::LoadOptions options/*=hkSerializeUtil::LOAD_DEFAULT*/)
{
	__try
	{
		return hkSerializeUtil::load(stream, detailsOut, options);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		if (detailsOut == NULL)
			detailsOut->id = hkSerializeUtil::ErrorDetails::ERRORID_LOAD_FAILED;
		return NULL;
	}
}

hkResult hkSerializeLoad(hkStreamReader *reader
	, hkVariant &root
	, hkResource *&resource)
{
	hkTypeInfoRegistry &defaultTypeRegistry = hkTypeInfoRegistry::getInstance();
	hkDefaultClassNameRegistry &defaultRegistry = hkDefaultClassNameRegistry::getInstance();

	hkBinaryPackfileReader bpkreader;
	hkXmlPackfileReader xpkreader;
	resource = NULL;
	hkSerializeUtil::FormatDetails formatDetails;
	hkSerializeUtil::detectFormat(reader, formatDetails);
	hkBool32 isLoadable = hkSerializeUtil::isLoadable(reader);
	if (!isLoadable && formatDetails.m_formatType != hkSerializeUtil::FORMAT_TAGFILE_XML)
	{
		return HK_FAILURE;
	}
	else
	{
		switch (formatDetails.m_formatType)
		{
		case hkSerializeUtil::FORMAT_PACKFILE_BINARY:
		{
			bpkreader.loadEntireFile(reader);
			bpkreader.finishLoadedObjects(defaultTypeRegistry);
			if (hkPackfileData* pkdata = bpkreader.getPackfileData())
			{
				hkArray<hkVariant>& obj = bpkreader.getLoadedObjects();
				for (int i = 0, n = obj.getSize(); i<n; ++i)
				{
					hkVariant& value = obj[i];
					if (value.m_class->hasVtable())
						defaultTypeRegistry.finishLoadedObject(value.m_object, value.m_class->getName());
				}
				resource = pkdata;
				resource->addReference();
			}
			root = bpkreader.getTopLevelObject();
		}
		break;

		case hkSerializeUtil::FORMAT_PACKFILE_XML:
		{
			xpkreader.loadEntireFile(reader);
			if (hkPackfileData* pkdata = xpkreader.getPackfileData())
			{
				hkArray<hkVariant>& obj = xpkreader.getLoadedObjects();
				for (int i = 0, n = obj.getSize(); i<n; ++i)
				{
					hkVariant& value = obj[i];
					if (value.m_class->hasVtable())
						defaultTypeRegistry.finishLoadedObject(value.m_object, value.m_class->getName());
				}
				resource = pkdata;
				resource->addReference();
				root = xpkreader.getTopLevelObject();
			}
		}
		break;

		case hkSerializeUtil::FORMAT_TAGFILE_BINARY:
		case hkSerializeUtil::FORMAT_TAGFILE_XML:
		default:
		{
			hkSerializeUtil::ErrorDetails detailsOut;
			hkSerializeUtil::LoadOptions loadflags = hkSerializeUtil::LOAD_FAIL_IF_VERSIONING;
			resource = hkSerializeUtilLoad(reader, &detailsOut, &defaultRegistry, loadflags);
			root.m_object = resource->getContents<hkRootLevelContainer>();
			if (root.m_object != NULL)
				root.m_class = &((hkRootLevelContainer*)root.m_object)->staticClass();
		}
		break;
		}
	}
	return root.m_object != NULL ? HK_SUCCESS : HK_FAILURE;
}

void scaleHkxAnimationDuration(std::string sourceXmlPath, std::string outputXmlPath, float durationScale){
	printf("Creating file: %s ...\n", outputXmlPath.c_str());

	// Open file

	hkIstream stream(sourceXmlPath.c_str());
	hkStreamReader* reader = stream.getStreamReader();
	hkVariant root;
	hkResource* resource;

	hkResult result = hkSerializeLoad(reader, root, resource);
	if(!result.isSuccess()){
		printf("Invalid file: %s\n", sourceXmlPath.c_str());
		return;
	}

	// Copy stuff we need and then delete the originals

	hkRootLevelContainer* rootContainer = resource->getContents<hkRootLevelContainer>();
	hkaAnimationContainer* animContainer = rootContainer->findObject<hkaAnimationContainer>();
	hkaAnimation* animation = animContainer->m_bindings[0]->m_animation;
	hkaAnimationBinding* binding = animContainer->m_bindings[0];

	hkArray<hkInt16> boneIndicesCopy;
	boneIndicesCopy = binding->m_transformTrackToBoneIndices;

	std::string originalSkeletonName = binding->m_originalSkeletonName;

	hkaInterleavedUncompressedAnimation* originalAnim = new hkaInterleavedUncompressedAnimation(*animation);

	int boneCount = originalAnim->m_numberOfTransformTracks;

	rootContainer->m_namedVariants.clear();
	delete animContainer->m_bindings[0];
	delete animContainer;

	// Create our new stuff

	hkaInterleavedUncompressedAnimation* newAnim = new hkaInterleavedUncompressedAnimation();

	newAnim->m_duration = originalAnim->m_duration * durationScale;
	newAnim->m_annotationTracks = originalAnim->m_annotationTracks;
	newAnim->m_numberOfTransformTracks = boneCount;

	int frameCount = 0;

	// Scale frames

	hkLocalArray<hkQsTransform> frameSample(boneCount);
	frameSample.setSize(boneCount);
	const float frameRate = 30.0f;
	const float frameDuration = 1.0f / frameRate;
	float duration = newAnim->m_duration - frameDuration * 0.1f;
	for (float time = 0.0f; time <= duration; time += frameDuration) {
		originalAnim->sampleTracks(time / durationScale, frameSample.begin(), NULL);
		newAnim->m_transforms.append(frameSample);
		++frameCount;
	}

	// Scale motion

	const hkaDefaultAnimatedReferenceFrame* originalMotion = static_cast<const hkaDefaultAnimatedReferenceFrame*>(originalAnim->getExtractedMotion());

	hkLocalArray<hkVector4> motionFrames(frameCount);
	motionFrames.setSize(frameCount);
	hkQsTransform motionFrame;

	hkaDefaultAnimatedReferenceFrame* newMotion = new hkaDefaultAnimatedReferenceFrame();
	newMotion->m_duration = originalMotion->getDuration() * durationScale;
	newMotion->m_forward = originalMotion->m_forward;
	newMotion->m_up = originalMotion->m_up;
	newMotion->m_frameType = originalMotion->m_frameType;
	newAnim->setExtractedMotion(newMotion);

	for (int n = 0; n < frameCount; ++n) {
		float originalTimeRatio = (float)n / (float)(frameCount - 1);
		float originalTime = originalAnim->m_duration * originalTimeRatio;

		originalAnim->getExtractedMotionReferenceFrame(originalTime, motionFrame);
		hkVector4 vec = motionFrame.getTranslation();
		motionFrames[n] = vec;
	}
	newMotion->m_referenceFrameSamples = motionFrames;
	
	// Apply
	hkaSplineCompressedAnimation* newAnimCompressed = new hkaSplineCompressedAnimation(*newAnim);

	binding = new hkaAnimationBinding();
	binding->m_animation = newAnimCompressed;
	binding->m_originalSkeletonName = originalSkeletonName.c_str();
	binding->m_transformTrackToBoneIndices = boneIndicesCopy;

	animContainer = new hkaAnimationContainer();
	animContainer->m_animations.pushBack(newAnimCompressed);
	animContainer->m_bindings.pushBack(binding);

	hkRootLevelContainer::NamedVariant variant = hkRootLevelContainer::NamedVariant(
		"Merged Animation Container",
		animContainer,
		&hkaAnimationContainer::staticClass()
	);
	rootContainer->m_namedVariants.pushBack(variant);

	// Save the new file

	hkXmlPackfileWriter xmlWriter;
	xmlWriter.setContents(rootContainer, hkRootLevelContainer::staticClass());

	hkOstream* ostream = new hkOstream(outputXmlPath.c_str());

	hkPackfileWriter::Options options;
	options.m_contentsVersion = "hk_2010.2.0-r1";

	result = xmlWriter.save(ostream->getStreamWriter(), options);
	if(result.isSuccess()){
		printf("Saved file: %s\n", outputXmlPath.c_str());
	}else{
		printf("Error saving file: %s\n", outputXmlPath.c_str());
		return;
	}

	delete ostream;

	// Hack: manually replace a value because Havok is 2 complex 4 me

	FILE* file = fopen(outputXmlPath.c_str(), "rb+");

	fseek(file, 0, SEEK_END);
	int fileLength = ftell(file);
	fseek(file, 0, SEEK_SET);

	std::string text = std::string(fileLength, '\0');
	fread(&text[0], 1, fileLength, file);

	size_t findResult = text.find("classversion=\"9\"");
	std::string replacement = "classversion=\"8\"";
	if(findResult != std::string::npos){
		text.replace(findResult, replacement.size(), replacement);
	}

	freopen(outputXmlPath.c_str(), "wb", file);

	fwrite(text.c_str(), 1, fileLength, file);

	fclose(file);
}

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
	int offsets[3];
	float startTime;
	float endTime;
	int type;
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

std::wstring readNameW(FILE* file, int offset) {
	long posBuffer = ftell(file);

	fseek(file, offset, SEEK_SET);

	std::wstring text;
	while (feof(file) == 0) {
		wchar_t c;
		fread(&c, 1, sizeof(wchar_t), file);

		if (c == L'\0') {
			break;
		}

		text += c;
	}

	fseek(file, posBuffer, SEEK_SET);

	return text;
}

std::wstring readNameW(FILE* file) {
	int offset;
	fread(&offset, 1, sizeof(int), file);

	return readNameW(file, offset);
}

// Special thanks to Nyxojaele's 010 templates for making this easy for me.
TaeFile* readTaeFile(FILE* file) {
	TaeFile* taeFile = new TaeFile;
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

		if (animHeader.eventCount == 0) {
			continue;
		}

		fseek(file, animHeader.eventOffset, SEEK_SET);

		animData.events = std::vector<Event>();
		for(int e = 0; e < animHeader.eventCount; ++e){
			Event event;

			fread(event.offsets, sizeof(event.offsets), 1, file);

			long posBuffer = ftell(file);
			{
				fseek(file, event.offsets[0], SEEK_SET);
				fread(&event.startTime, sizeof(float), 1, file);
				fseek(file, event.offsets[1], SEEK_SET);
				fread(&event.endTime, sizeof(float), 1, file);
				fseek(file, event.offsets[2], SEEK_SET);
				fread(&event.type, sizeof(int), 1, file);
				}
			}
			fseek(file, posBuffer, SEEK_SET);

			animData.events.push_back(event);
		}

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
		}
		else {
			printf("Unknown type: %n\n", animFile.type);
			throw new std::exception();
		}
		animData.animFile = animFile;
	}

	return taeFile;
}

void scaleTaeAnimationDuration(std::wstring sourceTaePath, std::wstring animFileName, float scale) {
	FILE* file = _wfopen(sourceTaePath.c_str(), L"rb+");

	if (file == NULL) {
		printf("Cannot open file: %s\n", sourceTaePath.c_str());
		return;
	}

	TaeFile* taeFile = readTaeFile(file);

	byte* bytes = NULL;
	int fileSize = 0;
	std::vector<Event>* events = NULL;

	std::transform(animFileName.begin(), animFileName.end(), animFileName.begin(), towlower);

	bool found = false;
	for (size_t n = 0; n < taeFile->animData.size(); ++n) {
		std::wstring nameLowercase = taeFile->animData[n].animFile.name;
		std::transform(nameLowercase.begin(), nameLowercase.end(), nameLowercase.begin(), towlower);

		std::wstring nameWithoutWin = nameLowercase;
		if (nameWithoutWin.size() > 3) {
			nameWithoutWin.resize(nameWithoutWin.size() - 3);
		}

		if (nameLowercase == animFileName || nameWithoutWin == animFileName) {
			found = true;

			events = &taeFile->animData[n].events;

			fileSize = taeFile->header.fileSize;
			bytes = new byte[fileSize];
			fseek(file, 0, SEEK_SET);
			fread(bytes, 1, fileSize, file);

			break;
		}
	}

	if (found) {
		std::wstring destTaePath = sourceTaePath + L".out";

		wprintf_s(L"Found anim file, saving to %s...\n", destTaePath.c_str());

		std::vector<int> offsets;

		for (size_t e = 0; e < events->size(); ++e) {
			offsets.push_back((*events)[e].offsets[0]);
			offsets.push_back((*events)[e].offsets[1]);
		}

		std::vector<int>::iterator it = std::unique(offsets.begin(), offsets.end());
		offsets.resize(std::distance(offsets.begin(), it));

		for (size_t n = 0; n < offsets.size(); ++n){
			*reinterpret_cast<float*>(&bytes[offsets[n]]) *= scale;
		}

		_wfreopen(destTaePath.c_str(), L"wb", file);

		fwrite(bytes, 1, fileSize, file);

		printf("Done!\n");
	}else{
		_wprintf_p(L"Did not find %s in %s \n", animFileName.c_str(), sourceTaePath.c_str());
	}

	fclose(file);
}

void taeTool(int argCount, const wchar_t** args) {
	const char* usageString =
		"Usage: tae <command> <args> \n\n"
		"Commands: scaleDuration(string taePath, string animFileName, float scale) \n\n"
		"Example: tae scaleDuration c5260.tae a00_3004.hkx 0.25 \n\n"
	;

	if (argCount < 4 || argCount > 4) {
		printf("%s", usageString);
		return;
	}

	std::wstring command = args[0];
	std::transform(command.begin(), command.end(), command.begin(), towlower);
	if (command == L"scaleduration") {
		std::wstring taePath = args[1];
		std::wstring fileName = args[2];
		float scale = (float)_wtof(args[3]);
		scaleTaeAnimationDuration(taePath, fileName, scale);
	}
	else {
		printf("%s", usageString);
		return;
	}
}

int wmain(int argCount, const wchar_t** args)
{
	printf("Hello\n");

	hkMemoryRouter* memoryRouter = hkMemoryInitUtil::initDefault(hkMallocAllocator::m_defaultMallocAllocator, hkMemorySystem::FrameInfo(1024 * 1024));
	hkBaseSystem::init(memoryRouter, havokErrorReport);

	std::string sourceXmlPath = "C:/Projects/Dark Souls/Anim research/a00_3004.orig.hkx.xml";
	std::string outputXmlPath = "C:/Projects/Dark Souls/Anim research/output.hkx.xml";
	// scaleHkxAnimationDuration(sourceXmlPath, outputXmlPath, 0.25f);

	// std::string sourceTaePath = "C:/Projects/Dark Souls/Anim research/c5260.orig.tae";
	// std::string outputTaePath = "C:/Projects/Dark Souls/Anim research/output.tae";

	for (int n = 0; n < argCount; ++n) {
		std::wstring arg = args[n];

		if (arg == L"tae") {
			taeTool(argCount - n - 1, &args[n + 1]);
		}
	}

	int unused;
	std::cin >> unused;

    return 0;
}
