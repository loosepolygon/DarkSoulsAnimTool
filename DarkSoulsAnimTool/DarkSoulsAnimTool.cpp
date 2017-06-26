
#include "stdafx.h"

#include <string>
#include <iostream>
#include <cstdio>

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

	hkaInterleavedUncompressedAnimation* originalAnimRaw = new hkaInterleavedUncompressedAnimation(*animation);

	rootContainer->m_namedVariants.clear();
	delete animContainer->m_bindings[0];
	delete animContainer;

	// Create our new stuff

	hkaInterleavedUncompressedAnimation* newAnimRaw = new hkaInterleavedUncompressedAnimation(*originalAnimRaw);
	hkaSplineCompressedAnimation* newAnim = new hkaSplineCompressedAnimation(*newAnimRaw);

	binding = new hkaAnimationBinding();
	binding->m_animation = newAnim;
	binding->m_originalSkeletonName = originalSkeletonName.c_str();
	binding->m_transformTrackToBoneIndices = boneIndicesCopy;

	animContainer = new hkaAnimationContainer();
	animContainer->m_animations.pushBack(newAnim);
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

int main(int argc, const char* args)
{
	printf("Hello\n");

	hkMemoryRouter* memoryRouter = hkMemoryInitUtil::initDefault(hkMallocAllocator::m_defaultMallocAllocator, hkMemorySystem::FrameInfo(1024 * 1024));
	hkBaseSystem::init(memoryRouter, havokErrorReport);

	std::string sourceXmlPath = "C:/Projects/Dark Souls/Anim research/a00_3004.orig.hkx.xml";
	std::string outputXmlPath = "C:/Projects/Dark Souls/Anim research/output.hkx.xml";

	scaleHkxAnimationDuration(sourceXmlPath, outputXmlPath, 0.25f);

	int unused;
	std::cin >> unused;

    return 0;
}
