#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/Atom/Feature/CoreLights/CapsuleLightFeatureProcessorInterface.h
    Include/Atom/Feature/CoreLights/DirectionalLightFeatureProcessorInterface.h
    Include/Atom/Feature/CoreLights/DiskLightFeatureProcessorInterface.h
    Include/Atom/Feature/CoreLights/EsmShadowmapsPassData.h
    Include/Atom/Feature/CoreLights/PhotometricValue.h
    Include/Atom/Feature/CoreLights/PointLightFeatureProcessorInterface.h
    Include/Atom/Feature/CoreLights/PolygonLightFeatureProcessorInterface.h
    Include/Atom/Feature/CoreLights/QuadLightFeatureProcessorInterface.h
    Include/Atom/Feature/CoreLights/SimplePointLightFeatureProcessorInterface.h
    Include/Atom/Feature/CoreLights/SimpleSpotLightFeatureProcessorInterface.h
    Include/Atom/Feature/CoreLights/ShadowConstants.h
    Include/Atom/Feature/CubeMapCapture/CubeMapCaptureFeatureProcessorInterface.h
    Include/Atom/Feature/Debug/RenderDebugConstants.h
    Include/Atom/Feature/Debug/RenderDebugFeatureProcessorInterface.h
    Include/Atom/Feature/Debug/RenderDebugParams.inl
    Include/Atom/Feature/Debug/RenderDebugSettingsInterface.h
    Include/Atom/Feature/Decals/DecalFeatureProcessorInterface.h
    Include/Atom/Feature/DisplayMapper/DisplayMapperFeatureProcessorInterface.h
    Include/Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h
    Include/Atom/Feature/Mesh/MeshCommon.h
    Include/Atom/Feature/Mesh/MeshFeatureProcessorInterface.h
    Include/Atom/Feature/MorphTargets/MorphTargetInputBuffers.h
    Include/Atom/Feature/ParamMacros/EndParams.inl
    Include/Atom/Feature/ParamMacros/MapAllCommon.inl
    Include/Atom/Feature/ParamMacros/MapOverrideCommon.inl
    Include/Atom/Feature/ParamMacros/MapOverrideEmpty.inl
    Include/Atom/Feature/ParamMacros/MapParamCommon.inl
    Include/Atom/Feature/ParamMacros/MapParamEmpty.inl
    Include/Atom/Feature/ParamMacros/ParamMacrosHowTo.inl
    Include/Atom/Feature/ParamMacros/StartOverrideBlend.inl
    Include/Atom/Feature/ParamMacros/StartOverrideEditorContext.inl
    Include/Atom/Feature/ParamMacros/StartParamBehaviorContext.inl
    Include/Atom/Feature/ParamMacros/StartParamCopySettingsFrom.inl
    Include/Atom/Feature/ParamMacros/StartParamCopySettingsTo.inl
    Include/Atom/Feature/ParamMacros/StartParamFunctions.inl
    Include/Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl
    Include/Atom/Feature/ParamMacros/StartParamFunctionsOverrideImpl.inl
    Include/Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl
    Include/Atom/Feature/ParamMacros/StartParamMembers.inl
    Include/Atom/Feature/ParamMacros/StartParamSerializeContext.inl
    Include/Atom/Feature/OcclusionCullingPlane/OcclusionCullingPlaneFeatureProcessorInterface.h
    Include/Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h
    Include/Atom/Feature/PostProcess/PostProcessParams.inl
    Include/Atom/Feature/PostProcess/PostProcessSettings.inl
    Include/Atom/Feature/PostProcess/PostProcessSettingsInterface.h
    Include/Atom/Feature/PostProcess/Bloom/BloomConstants.h
    Include/Atom/Feature/PostProcess/Bloom/BloomParams.inl
    Include/Atom/Feature/PostProcess/Bloom/BloomSettingsInterface.h
    Include/Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationConstants.h
    Include/Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationParams.inl
    Include/Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationSettingsInterface.h
    Include/Atom/Feature/PostProcess/PaniniProjection/PaniniProjectionConstants.h
    Include/Atom/Feature/PostProcess/PaniniProjection/PaniniProjectionParams.inl
    Include/Atom/Feature/PostProcess/PaniniProjection/PaniniProjectionSettingsInterface.h
    Include/Atom/Feature/PostProcess/FilmGrain/FilmGrainConstants.h
    Include/Atom/Feature/PostProcess/FilmGrain/FilmGrainParams.inl
    Include/Atom/Feature/PostProcess/FilmGrain/FilmGrainSettingsInterface.h
    Include/Atom/Feature/PostProcess/Vignette/VignetteConstants.h
    Include/Atom/Feature/PostProcess/Vignette/VignetteParams.inl
    Include/Atom/Feature/PostProcess/Vignette/VignetteSettingsInterface.h
    Include/Atom/Feature/PostProcess/ColorGrading/HDRColorGradingParams.inl
    Include/Atom/Feature/PostProcess/ColorGrading/HDRColorGradingSettingsInterface.h
    Include/Atom/Feature/PostProcess/DepthOfField/DepthOfFieldConstants.h
    Include/Atom/Feature/PostProcess/DepthOfField/DepthOfFieldParams.inl
    Include/Atom/Feature/PostProcess/DepthOfField/DepthOfFieldSettingsInterface.h
    Include/Atom/Feature/PostProcess/ExposureControl/ExposureControlConstants.h
    Include/Atom/Feature/PostProcess/ExposureControl/ExposureControlParams.inl
    Include/Atom/Feature/PostProcess/ExposureControl/ExposureControlSettingsInterface.h
    Include/Atom/Feature/PostProcess/Ssao/SsaoConstants.h
    Include/Atom/Feature/PostProcess/Ssao/SsaoParams.inl
    Include/Atom/Feature/PostProcess/Ssao/SsaoSettingsInterface.h
    Include/Atom/Feature/PostProcess/LookModification/LookModificationParams.inl
    Include/Atom/Feature/PostProcess/LookModification/LookModificationSettingsInterface.h
    Include/Atom/Feature/PostProcess/WhiteBalance/WhiteBalanceConstants.h
    Include/Atom/Feature/PostProcess/WhiteBalance/WhiteBalanceParams.inl
    Include/Atom/Feature/PostProcess/WhiteBalance/WhiteBalanceSettingsInterface.h
    Include/Atom/Feature/ScreenSpace/DeferredFogSettingsInterface.h
    Include/Atom/Feature/ScreenSpace/DeferredFogParams.inl
    Include/Atom/Feature/ReflectionProbe/ReflectionProbeFeatureProcessorInterface.h
    Include/Atom/Feature/Shadows/ProjectedShadowFeatureProcessorInterface.h
    Include/Atom/Feature/SkinnedMesh/SkinnedMeshFeatureProcessorBus.h
    Include/Atom/Feature/SkinnedMesh/SkinnedMeshFeatureProcessorInterface.h
    Include/Atom/Feature/SkinnedMesh/SkinnedMeshInputBuffers.h
    Include/Atom/Feature/SkinnedMesh/SkinnedMeshInstance.h
    Include/Atom/Feature/SkinnedMesh/SkinnedMeshOutputStreamManagerInterface.h
    Include/Atom/Feature/SkinnedMesh/SkinnedMeshStatsBus.h
    Include/Atom/Feature/SkinnedMesh/SkinnedMeshVertexStreams.h
    Include/Atom/Feature/SkinnedMesh/SkinnedMeshShaderOptions.h
    Include/Atom/Feature/SplashScreen/SplashScreenSettings.h
    Include/Atom/Feature/TransformService/TransformServiceFeatureProcessorInterface.h
    Include/Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h
    Include/Atom/Feature/SkyAtmosphere/SkyAtmosphereFeatureProcessorInterface.h
    Source/CoreLights/PhotometricValue.cpp
    Source/MorphTargets/MorphTargetInputBuffers.cpp
    Source/SkinnedMesh/SkinnedMeshInputBuffers.cpp
    Source/SkinnedMesh/SkinnedMeshInstance.cpp
    Source/SplashScreen/SplashScreenSettings.cpp
)
