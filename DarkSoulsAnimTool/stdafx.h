// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Havok includes

/*
// [id=keycode]
#include <Common/Base/keycode.cxx>

#undef HK_FEATURE_PRODUCT_AI
//#undef HK_FEATURE_PRODUCT_ANIMATION
#undef HK_FEATURE_PRODUCT_CLOTH
#undef HK_FEATURE_PRODUCT_DESTRUCTION
#undef HK_FEATURE_PRODUCT_BEHAVIOR
#undef HK_FEATURE_PRODUCT_PHYSICS

// Ai
#define HK_EXCLUDE_LIBRARY_hkaiAiPhysicsBridge

// Animation
#define HK_EXCLUDE_LIBRARY_hkaRagdoll

// Behavior
#define HK_EXCLUDE_LIBRARY_hkbUtilities

// Cloth
#define HK_EXCLUDE_LIBRARY_hclPhysicsBridge
#define HK_EXCLUDE_LIBRARY_hclSetup
#define HK_EXCLUDE_LIBRARY_hclAnimationBridge

// Physics
#define HK_EXCLUDE_LIBRARY_hkpUtilities
#define HK_EXCLUDE_LIBRARY_hkpVehicle

// Common
#define HK_EXCLUDE_LIBRARY_hkSceneData
#define HK_EXCLUDE_LIBRARY_hkVisualize
#define HK_EXCLUDE_LIBRARY_hkGeometryUtilities
// #define HK_EXCLUDE_LIBRARY_hkCompat

// Convex Decomposition
#define HK_EXCLUDE_LIBRARY_hkgpConvexDecomposition

#define HK_EXCLUDE_FEATURE_hkMonitorStream
#define HK_EXCLUDE_FEATURE_hkpAabbTreeWorldManager
#define HK_EXCLUDE_FEATURE_hkpAccurateInertiaTensorComputer
#define HK_EXCLUDE_FEATURE_hkpCompressedMeshShape
#define HK_EXCLUDE_FEATURE_hkpContinuousSimulation
#define HK_EXCLUDE_FEATURE_hkpConvexPieceMeshShape
#define HK_EXCLUDE_FEATURE_hkpExtendedMeshShape
#define HK_EXCLUDE_FEATURE_hkpHeightField
#define HK_EXCLUDE_FEATURE_hkpKdTreeWorldManager
#define HK_EXCLUDE_FEATURE_hkpMeshShape
#define HK_EXCLUDE_FEATURE_hkpMultiThreadedSimulation
#define HK_EXCLUDE_FEATURE_hkpPoweredChainData
#define HK_EXCLUDE_FEATURE_hkpSimpleMeshShape
#define HK_EXCLUDE_FEATURE_hkpSimulation

#include <Common/Base/Config/hkProductFeatures.cxx>


#include <Common/Base/hkBase.h>
#include <Common/Base/Container/Array/hkArray.h>
#include <Common/Base/Memory/System/Util/hkMemoryInitUtil.h>
#include <Common/Base/Memory/Allocator/Malloc/hkMallocAllocator.h>
#include <Common/Base/System/Io/IStream/hkIStream.h>
#include <Common/Base/System/Io/OStream/hkOStream.h>
#include <Common/Base/Reflection/Registry/hkDynamicClassNameRegistry.h>
#include <Common/Base/Reflection/Registry/hkDefaultClassNameRegistry.h>
#include <Common/Compat/Deprecated/Packfile/Binary/hkBinaryPackfileReader.h>
#include <Common/Compat/Deprecated/Packfile/Xml/hkXmlPackfileReader.h>
#include <Common/Compat/Deprecated/Packfile/Xml/hkXmlPackfileWriter.h>
#include <Common/Serialize/Util/hkSerializeUtil.h>
#include <Common/Serialize/Util/hkRootLevelContainer.h>
#include <Common/Serialize/Util/hkNativePackfileUtils.h>
#include <Common/Serialize/Util/hkLoader.h>

// Animation
#include <Animation/Animation/hkaAnimationContainer.h>
#include <Animation/Animation/Animation/Interleaved/hkaInterleavedUncompressedAnimation.h>
#include <Animation/Animation/Animation/SplineCompressed/hkaSplineCompressedAnimation.h>
#include <Animation/Animation/Motion/Default/hkaDefaultAnimatedReferenceFrame.h>


// We are using serialization, so we need ReflectedClasses.
// The objects are being saved and then loaded immediately so we know the version of the saved data is the same
// as the version the application is linked with. Because of this we don't need RegisterVersionPatches or SerializeDeprecatedPre700.
// If the demo was reading content saved from a previous version of the Havok content tools (common in real world Applications)
// RegisterVersionPatches and perhaps SerializeDeprecatedPre700 are needed.

//#define HK_EXCLUDE_FEATURE_SerializeDeprecatedPre700

// We can also restrict the compatibility to files created with the current version only using HK_SERIALIZE_MIN_COMPATIBLE_VERSION.
// If we wanted to have compatibility with at most version 650b1 we could have used something like:
// #define HK_SERIALIZE_MIN_COMPATIBLE_VERSION 650b1.
#define HK_SERIALIZE_MIN_COMPATIBLE_VERSION HK_HAVOK_VERSION_201010r1

//#define HK_EXCLUDE_FEATURE_RegisterVersionPatches
//#define HK_EXCLUDE_FEATURE_RegisterReflectedClasses
//#define HK_EXCLUDE_FEATURE_MemoryTracker


// Platform specific initialization
// #include <Common/Base/System/Init/PlatformInit.cxx>

*/
