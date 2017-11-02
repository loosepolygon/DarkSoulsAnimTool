#include "stdafx.h"

#include "structs.hpp"

#include <iostream>
#include <algorithm>
#include <queue>

/*
void HK_CALL havokErrorReport(const char* msg, void* userContext){
	printf("%s\n", msg);
}

hkResource* hkSerializeUtilLoad(hkStreamReader* stream
	, hkSerializeUtil::ErrorDetails* detailsOut
	, const hkClassNameRegistry* classReg
	, hkSerializeUtil::LoadOptions options)
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

void scaleHkxAnimationDuration(std::string sourceXmlPath, std::string outputXmlPath, float durationScale) {
   printf("Creating file: %s ...\n", outputXmlPath.c_str());

   // Open file

   hkIstream stream(sourceXmlPath.c_str());
   hkStreamReader* reader = stream.getStreamReader();
   hkVariant root;
   hkResource* resource;

   hkResult result = hkSerializeLoad(reader, root, resource);
   if (!result.isSuccess()) {
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
   if (result.isSuccess()) {
      printf("Saved file: %s\n", outputXmlPath.c_str());
   }
   else {
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
   if (findResult != std::string::npos) {
      text.replace(findResult, replacement.size(), replacement);
   }

   freopen(outputXmlPath.c_str(), "wb", file);

   fwrite(text.c_str(), 1, fileLength, file);

   fclose(file);
}
*/

void checkEmpty(std::queue<std::wstring>& words) {
   if (words.empty()) {
      printf("Not enough args\n");
      exit(1);
   }
}

std::wstring popString(std::queue<std::wstring>& words) {
   checkEmpty(words);

   std::wstring result = words.front();
   words.pop();
   return result;
}

float popFloat(std::queue<std::wstring>& words) {
   checkEmpty(words);

   float result = (float)_wtof(words.front().c_str());
   words.pop();
   return result;
}

int wmain(int argCount, const wchar_t** args)
{
	//hkMemoryRouter* memoryRouter = hkMemoryInitUtil::initDefault(hkMallocAllocator::m_defaultMallocAllocator, hkMemorySystem::FrameInfo(1024 * 1024));
	//hkBaseSystem::init(memoryRouter, havokErrorReport);

	//std::string sourceXmlPath = "C:/Projects/Dark Souls/Anim research/a00_3004.orig.hkx.xml";
	//std::string outputXmlPath = "C:/Projects/Dark Souls/Anim research/output.hkx.xml";
	// scaleHkxAnimationDuration(sourceXmlPath, outputXmlPath, 0.25f);

	// std::string sourceTaePath = "C:/Projects/Dark Souls/Anim research/c5260.orig.tae";
	// std::string outputTaePath = "C:/Projects/Dark Souls/Anim research/output.tae";


   std::queue<std::wstring> words;
   for (int n = 1; n < argCount; ++n) {
      words.push(args[n]);
   }

   checkEmpty(words);

   std::wstring command = popString(words);
   std::transform(command.begin(), command.end(), command.begin(), towlower);

	for (int n = 0; n < argCount; ++n) {
		if (command == L"scaleanim") {
         /*scaleAnim(
            popString(words),
            popString(words),
            popFloat(words)
         );*/
         // C++ provides no guarantee that the two popStrings will be evaluated before the
         // popFloat, it's undefined. I hate this language so much.

         std::wstring s1 = popString(words);
         std::wstring s2 = popString(words);
         float f1 = popFloat(words);
         scaleAnim(s1, s2, f1);
      }else if (command == L"exporttae") {
         std::wstring s1 = popString(words);
         exportTae(s1);
      }
	}

	int unused;
	std::cin >> unused;

    return 0;
}
