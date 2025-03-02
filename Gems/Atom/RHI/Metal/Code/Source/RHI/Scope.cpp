/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/ResolveScopeAttachment.h>
#include <Atom/RHI/SwapChain.h>
#include <RHI/CommandList.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/ImageView.h>
#include <RHI/ResourcePoolResolver.h>
#include <RHI/Scope.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>


namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<Scope> Scope::Create()
        {
            return aznew Scope();
        }
    
        void Scope::InitInternal()
        {
            m_markerName = Name(GetMarkerLabel());
        }
    
        void Scope::DeactivateInternal()
        {
            for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
            {
                static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Deactivate();
            }
            m_waitFencesByQueue = { {0} };
            m_signalFenceValue = 0;
            m_renderPassDescriptor = nil;
            
            for (size_t i = 0; i < static_cast<uint32_t>(ResourceFenceAction::Count); ++i)
            {
                m_resourceFences[i].clear();
            }
            m_queryPoolAttachments.clear();
            m_residentHeaps.clear();

        }

        void Scope::ApplyMSAACustomPositions(const ImageView* imageView)
        {
            const Image& image = imageView->GetImage();
            const RHI::ImageDescriptor& imageDescriptor = image.GetDescriptor();
            
            m_scopeMultisampleState = imageDescriptor.m_multisampleState;
            if(m_scopeMultisampleState.m_customPositionsCount > 0)
            {
                NSUInteger numSampleLocations = m_scopeMultisampleState.m_samples;
                AZStd::vector<MTLSamplePosition> mtlCustomSampleLocations;
                AZStd::transform( m_scopeMultisampleState.m_customPositions.begin(),
                                 m_scopeMultisampleState.m_customPositions.begin() + numSampleLocations,
                                 AZStd::back_inserter(mtlCustomSampleLocations), [&](const auto& item)
                {
                    return ConvertSampleLocation(item);
                });
                [m_renderPassDescriptor setSamplePositions:mtlCustomSampleLocations.data() count:numSampleLocations];
            }
        }
    
        void Scope::CompileInternal()
        {
            RHI::Device& device = GetDevice();
            Device& mtlDevice = static_cast<Device&>(device);
            for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
            {
                static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Compile();
            }
            
            if(GetImageAttachments().size() > 0)
            {
                AZ_Assert(m_renderPassDescriptor == nil, "m_renderPassDescriptor should be null");
                m_renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
            }

            if(GetEstimatedItemCount())
            {
                m_residentHeaps.insert(mtlDevice.GetNullDescriptorManager().GetNullDescriptorHeap());
            }
            
            int colorAttachmentIndex = 0;
            AZStd::unordered_map<RHI::AttachmentId, ResolveAttachmentData> attachmentsIndex;
            
            for (RHI::ImageScopeAttachment* scopeAttachment : GetImageAttachments())
            {
                m_isWritingToSwapChainScope = 
                    scopeAttachment->IsSwapChainAttachment() &&
                    scopeAttachment->GetUsage() == RHI::ScopeAttachmentUsage::RenderTarget;
                const RHI::SwapChainFrameAttachment* swapChainFrameAttachment = (azrtti_cast<const RHI::SwapChainFrameAttachment*>(&scopeAttachment->GetFrameAttachment()));
                
                //Check if this scope is writing to the swapchain texture and if the swapchain attachment is valid
                if(isWritingToSwapChainScope && swapChainFrameAttachment)
                {
                    RHI::ScopeAttachment* frameCaptureScopeAttachment = scopeAttachment;
                    
                    //The way Metal works is that we ask the drivers for the swapchain texture right before we write to it.
                    //Check if we are the first scope in the chain of scope that will be writing to the swapchain texture.
                    //We will use this to request the drawable from the driver in the Execute phase (i.e Scope::Begin)
                    RHI::ScopeAttachment* frameCaptureScopeAttachmentBefore = frameCaptureScopeAttachment->GetPrevious();
                    if(!frameCaptureScopeAttachmentBefore)
                    {
                        m_isRequestingSwapChainDrawable = true;
                    }
                        
                    //Cache if we will be writing to the swapchain texture which is different from requesting one. The difference
                    //is that a scope may just use the swapchain texture requested by another scope before it in the frame graph.
                    m_isWritingToSwapChainScope = true;
                    m_swapChainAttachment = scopeAttachment;
                    
                    //If a scope needs to read from the swapchain texture we need to tell the driver this information when requesting the
                    //texture. Traverse all the scopeattachments and see if any of them is trying to read from the swapchain texture.
                    //If it is we cache this information within m_isSwapChainAndFrameCaptureEnabled which will
                    //be used when we request the swapchain texture.
                    while(frameCaptureScopeAttachment)
                    {
                        frameCaptureScopeAttachment = frameCaptureScopeAttachment->GetNext();
                        if(frameCaptureScopeAttachment &&
                           frameCaptureScopeAttachment->GetUsage() == RHI::ScopeAttachmentUsage::Copy &&
                           frameCaptureScopeAttachment->GetAccess() == RHI::ScopeAttachmentAccess::Read)
                        {
                            m_isSwapChainAndFrameCaptureEnabled = true;
                            break;
                        }
                    }
                }
                
                const ImageView* imageView = static_cast<const ImageView*>(scopeAttachment->GetImageView()->GetDeviceImageView(GetDeviceIndex()).get());
                const RHI::ImageScopeAttachmentDescriptor& bindingDescriptor = scopeAttachment->GetDescriptor();
                id<MTLTexture> imageViewMtlTexture = imageView->GetMemoryView().GetGpuAddress<id<MTLTexture>>();
                
                const bool isClearAction        = bindingDescriptor.m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear;
                const bool isClearActionStencil = bindingDescriptor.m_loadStoreAction.m_loadActionStencil == RHI::AttachmentLoadAction::Clear;
                
                const bool isLoadAction         = bindingDescriptor.m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Load;
                
                const bool isStoreAction         = bindingDescriptor.m_loadStoreAction.m_storeAction == RHI::AttachmentStoreAction::Store;
                const bool isStoreActionStencil  = bindingDescriptor.m_loadStoreAction.m_storeActionStencil == RHI::AttachmentStoreAction::Store;
                
                
                MTLLoadAction mtlLoadAction = MTLLoadActionDontCare;
                MTLStoreAction mtlStoreAction = MTLStoreActionDontCare;
                MTLLoadAction mtlLoadActionStencil = MTLLoadActionDontCare;
                MTLStoreAction mtlStoreActionStencil = MTLStoreActionDontCare;
                
                if(isClearAction)
                {
                    mtlLoadAction = MTLLoadActionClear;
                }
                else if(isLoadAction)
                {
                    mtlLoadAction = MTLLoadActionLoad;
                }
                
                if(isStoreAction)
                {
                    mtlStoreAction = MTLStoreActionStore;
                }
                
                if(isClearActionStencil)
                {
                    mtlLoadActionStencil = MTLLoadActionClear;
                }
                else if(isLoadAction)
                {
                    mtlLoadActionStencil = MTLLoadActionLoad;
                }
                
                if(isStoreActionStencil)
                {
                    mtlStoreActionStencil = MTLStoreActionStore;
                }

                switch (scopeAttachment->GetUsage())
                {
                    case RHI::ScopeAttachmentUsage::Shader:
                    {
                        break;
                    }
                    case RHI::ScopeAttachmentUsage::RenderTarget:
                    {
                        id<MTLTexture> renderTargetTexture = imageViewMtlTexture;
                        m_renderPassDescriptor.colorAttachments[colorAttachmentIndex].texture = renderTargetTexture;
                        
                        if(!m_isWritingToSwapChainScope)
                        {
                            //Cubemap/cubemaparray and 3d textures have restrictions placed on them by the
                            //drivers when creating a new texture view. Hence we cant get a view with subresource range
                            //of the original texture. As a result in order to write into specific slice or depth plane
                            //we specify it here. It also means that we cant write into these texturee types via a compute shader
                            const RHI::ImageViewDescriptor& imgViewDescriptor = imageView->GetDescriptor();
                            if(renderTargetTexture.textureType == MTLTextureTypeCube || renderTargetTexture.textureType == MTLTextureTypeCubeArray)
                            {
                                m_renderPassDescriptor.colorAttachments[colorAttachmentIndex].slice = imgViewDescriptor.m_arraySliceMin;
                            }
                            else if(renderTargetTexture.textureType == MTLTextureType3D)
                            {
                                m_renderPassDescriptor.colorAttachments[colorAttachmentIndex].depthPlane = imgViewDescriptor.m_depthSliceMin;
                            }
                        }
                        
                        MTLRenderPassColorAttachmentDescriptor* colorAttachment = m_renderPassDescriptor.colorAttachments[colorAttachmentIndex];
                        colorAttachment.loadAction = mtlLoadAction;
                        colorAttachment.storeAction = mtlStoreAction;

                        RHI::ClearValue clearVal = bindingDescriptor.m_loadStoreAction.m_clearValue;
                        if (mtlLoadAction == MTLLoadActionClear)
                        {
                            m_isClearNeeded = true;
                            if(clearVal.m_type == RHI::ClearValueType::Vector4Float)
                            {
                                colorAttachment.clearColor = MTLClearColorMake(clearVal.m_vector4Float[0], clearVal.m_vector4Float[1], clearVal.m_vector4Float[2], clearVal.m_vector4Float[3]);
                            }
                            else if(clearVal.m_type == RHI::ClearValueType::Vector4Uint)
                            {
                                colorAttachment.clearColor = MTLClearColorMake(clearVal.m_vector4Uint[0], clearVal.m_vector4Uint[1], clearVal.m_vector4Uint[2], clearVal.m_vector4Uint[3]);
                            }
                        }
                        
                        ApplyMSAACustomPositions(imageView);
                        attachmentsIndex[bindingDescriptor.m_attachmentId] = ResolveAttachmentData{colorAttachmentIndex, m_renderPassDescriptor, RHI::ScopeAttachmentUsage::RenderTarget, isStoreAction};
                        colorAttachmentIndex++;
                        break;
                    }
                    case RHI::ScopeAttachmentUsage::DepthStencil:
                    {
                        // We can have multiple DepthStencil attachments in order to specify depth and stencil access separately.
                        // One attachment is depth only, and the other is stencil only.
                        const RHI::ImageViewDescriptor& viewDescriptor = imageView->GetDescriptor();
                        if(RHI::CheckBitsAll(viewDescriptor.m_aspectFlags, RHI::ImageAspectFlags::Depth) ||
                           m_renderPassDescriptor.depthAttachment.texture == nil)
                        {
                            MTLRenderPassDepthAttachmentDescriptor* depthAttachment = m_renderPassDescriptor.depthAttachment;
                            depthAttachment.texture = imageViewMtlTexture;
                            depthAttachment.loadAction = mtlLoadAction;
                            depthAttachment.storeAction = mtlStoreAction;
                            if (bindingDescriptor.m_loadStoreAction.m_clearValue.m_type == RHI::ClearValueType::DepthStencil)
                            {
                                m_isClearNeeded = true;
                                depthAttachment.clearDepth = bindingDescriptor.m_loadStoreAction.m_clearValue.m_depthStencil.m_depth;
                            }
                        }
                        
                        // Set the stencil only if the format support it and we either have a null stencil or the attachment is stencil only.
                        if(IsDepthStencilMerged(imageView->GetSpecificFormat()) &&
                           (RHI::CheckBitsAll(viewDescriptor.m_aspectFlags, RHI::ImageAspectFlags::Stencil) ||
                            m_renderPassDescriptor.stencilAttachment.texture == nil))
                        {
                            MTLRenderPassStencilAttachmentDescriptor* stencilAttachment = m_renderPassDescriptor.stencilAttachment;
                            stencilAttachment.texture = imageViewMtlTexture;
                            stencilAttachment.loadAction = mtlLoadActionStencil;
                            stencilAttachment.storeAction = mtlStoreActionStencil;
                            if (bindingDescriptor.m_loadStoreAction.m_clearValue.m_type == RHI::ClearValueType::DepthStencil)
                            {
                                m_isClearNeeded = true;
                                stencilAttachment.clearStencil = bindingDescriptor.m_loadStoreAction.m_clearValue.m_depthStencil.m_stencil;
                            }
                        }
                        
                        ApplyMSAACustomPositions(imageView);
                        attachmentsIndex[bindingDescriptor.m_attachmentId] = ResolveAttachmentData{-1, m_renderPassDescriptor, RHI::ScopeAttachmentUsage::DepthStencil, isStoreAction};
                        
                        break;
                    }
                    case RHI::ScopeAttachmentUsage::Resolve:
                    {
                        auto resolveScopeAttachment = static_cast<const RHI::ResolveScopeAttachment*>(scopeAttachment);
                        auto resolveAttachmentId = resolveScopeAttachment->GetDescriptor().m_resolveAttachmentId;
                        AZ_Assert(attachmentsIndex.find(resolveAttachmentId) != attachmentsIndex.end(), "Failed to find resolvable attachment %s", resolveAttachmentId.GetCStr());
                        ResolveAttachmentData resolveAttachmentData = attachmentsIndex[resolveAttachmentId];
                        MTLRenderPassDescriptor* renderPassDesc = resolveAttachmentData.m_renderPassDesc;

                        id<MTLTexture> renderTargetTexture = imageViewMtlTexture;
                        
                        MTLStoreAction resolveStoreAction = resolveAttachmentData.m_isStoreAction ? MTLStoreActionStoreAndMultisampleResolve : MTLStoreActionMultisampleResolve;
                        
                        if(resolveAttachmentData.m_attachmentUsage == RHI::ScopeAttachmentUsage::RenderTarget)
                        {
                            renderPassDesc.colorAttachments[resolveAttachmentData.m_colorAttachmentIndex].resolveTexture = renderTargetTexture;
                            MTLRenderPassColorAttachmentDescriptor* colorAttachment = renderPassDesc.colorAttachments[resolveAttachmentData.m_colorAttachmentIndex];
                            colorAttachment.storeAction = resolveStoreAction;
                        }
                        else if (resolveAttachmentData.m_attachmentUsage == RHI::ScopeAttachmentUsage::DepthStencil)
                        {
                            renderPassDesc.depthAttachment.resolveTexture = renderTargetTexture;
                            MTLRenderPassDepthAttachmentDescriptor* depthAttachment = renderPassDesc.depthAttachment;
                            depthAttachment.storeAction = resolveStoreAction;
                            //Metal drivers support min/max depth resolve filters but there is no way to set them at a higher level yet.
                        }
                        m_isResolveNeeded = true;
                        break;
                    }
                }
            }
        }

        void Scope::Begin(
            CommandList& commandList,
            AZ::u32 commandListIndex,
            AZ::u32 commandListCount) const
        {
            AZ_PROFILE_FUNCTION(RHI);

            if(m_isWritingToSwapChainScope)
            {
                //Metal requires you to request for swapchain drawable as late as possible in the frame. Hence we call for the drawable
                //here and attach it directly to the colorAttachment. The assumption here is that this scope should be the
                //CopyToSwapChain scope. With this way if the internal resolution differs from the window resolution the compositer
                //within the metal driver will perform the appropriate upscale/downscale at the end.
                const RHI::DeviceSwapChain* swapChain = (azrtti_cast<const RHI::SwapChainFrameAttachment*>(&m_swapChainAttachment->GetFrameAttachment()))->GetSwapChain()->GetDeviceSwapChain(GetDeviceIndex()).get();
                SwapChain* metalSwapChain = static_cast<SwapChain*>(const_cast<RHI::DeviceSwapChain*>(swapChain));
                m_renderPassDescriptor.colorAttachments[0].texture = metalSwapChain->RequestDrawable(m_isSwapChainAndFrameCaptureEnabled);
            }
            
            const bool isPrologue = commandListIndex == 0;
            commandList.SetName(m_markerName);
            commandList.SetRenderPassInfo(m_renderPassDescriptor, m_scopeMultisampleState, m_residentHeaps);
            
            if (isPrologue)
            {
                //Check if the scope is part of merged group (i.e FrameGraphExecuteGroupMerged). For non-merged
                //groups (i.e FrameGraphExecuteGroup) we handle fence waiting at the start of the group within FrameGraphExecuteGroup::BeginInternal
                //The reason for this is that you can only wait on fences before the parallel encoders (within FrameGraphExecuteGroup) are created
                const bool isScopePartOfMergedGroup = commandListCount == 1;
                if (isScopePartOfMergedGroup)
                {
                    WaitOnAllResourceFences(commandList);
                }

                for (RHI::ResourcePoolResolver* resolvePolicyBase : GetResourcePoolResolves())
                {
                    static_cast<ResourcePoolResolver*>(resolvePolicyBase)->Resolve(commandList);
                }
            }
              
            for (const auto& queryPoolAttachment : m_queryPoolAttachments)
            {
                if (!RHI::CheckBitsAll(queryPoolAttachment.m_access, RHI::ScopeAttachmentAccess::Write))
                {
                    continue;
                }

                //Attach occlusion testing related visibility buffer
                if(queryPoolAttachment.m_pool->GetDescriptor().m_type == RHI::QueryType::Occlusion)
                {
                    commandList.AttachVisibilityBuffer(AZStd::static_pointer_cast<QueryPool>(queryPoolAttachment.m_pool->GetDeviceQueryPool(GetDeviceIndex()))->GetVisibilityBuffer());
                }
            }
            
            //If a scope has resolve texture or if it is using a clear load action we know the encoder type is Render
            //and hence we create the encoder here even though there may not be any draw commands by the scope. This will
            //allow us to use the driver to clear a render target or do a msaa resolve within a scope with no draw work.
            if(m_isResolveNeeded || m_isClearNeeded)
            {
                commandList.CreateEncoder(CommandEncoderType::Render);
            }
        }

        void Scope::End(
            CommandList& commandList,
            bool signalFencesForAliasedResources) const
        {
            AZ_PROFILE_FUNCTION(RHI);
            commandList.FlushEncoder();
            
            //Signal all the fences related to aliased resources. This is to ensure that the GPU does not start work for
            //scopes that overlap memory with other scopes that are still being worked on.
            if (signalFencesForAliasedResources)
            {
                SignalAllResourceFences(commandList);
            }
        }
        
        void Scope::SignalAllResourceFences(CommandList& commandList) const
        {
            for (const auto& fence : m_resourceFences[static_cast<int>(ResourceFenceAction::Signal)])
            {
                commandList.SignalResourceFence(fence);
            }
        }
    
        void Scope::SignalAllResourceFences(id <MTLCommandBuffer> mtlCommandBuffer) const
        {
            for (const auto& fence : m_resourceFences[static_cast<int>(ResourceFenceAction::Signal)])
            {
                fence.SignalFromGpu(mtlCommandBuffer);
            }
        }
    
        void Scope::WaitOnAllResourceFences(CommandList& commandList) const
        {
            for (const auto& fence : m_resourceFences[static_cast<int>(ResourceFenceAction::Wait)])
            {
                commandList.WaitOnResourceFence(fence);
            }
        }
    
        void Scope::WaitOnAllResourceFences(id <MTLCommandBuffer> mtlCommandBuffer) const
        {
            for (const auto& fence : m_resourceFences[static_cast<int>(ResourceFenceAction::Wait)])
            {
                fence.WaitOnGpu(mtlCommandBuffer);
            }
        }
    
        MTLRenderPassDescriptor* Scope::GetRenderPassDescriptor() const
        {
            return  m_renderPassDescriptor;
        }
    
        void Scope::SetSignalFenceValue(uint64_t fenceValue)
        {
            m_signalFenceValue = fenceValue;
        }

        bool Scope::HasSignalFence() const
        {
            return m_signalFenceValue > 0;
        }

        bool Scope::HasWaitFences() const
        {
            bool hasWaitFences = false;
            for (uint32_t i = 0; i < RHI::HardwareQueueClassCount; ++i)
            {
                hasWaitFences = hasWaitFences || (m_waitFencesByQueue[i] > 0);
            }
            return hasWaitFences;
        }

        uint64_t Scope::GetSignalFenceValue() const
        {
            return m_signalFenceValue;
        }

        void Scope::SetWaitFenceValueByQueue(RHI::HardwareQueueClass hardwareQueueClass, uint64_t fenceValue)
        {
            m_waitFencesByQueue[static_cast<uint32_t>(hardwareQueueClass)] = fenceValue;
        }

        uint64_t Scope::GetWaitFenceValueByQueue(RHI::HardwareQueueClass hardwareQueueClass) const
        {
            return m_waitFencesByQueue[static_cast<uint32_t>(hardwareQueueClass)];
        }

        const FenceValueSet& Scope::GetWaitFences() const
        {
            return m_waitFencesByQueue;
        }
    
        void Scope::QueueResourceFence(ResourceFenceAction fenceAction, Fence& fence)
        {
            m_resourceFences[static_cast<uint32_t>(fenceAction)].push_back(fence);
        }
    
        void Scope::AddQueryPoolUse(RHI::Ptr<RHI::QueryPool> queryPool, const RHI::Interval& interval, RHI::ScopeAttachmentAccess access)
        {
            m_queryPoolAttachments.push_back({ queryPool, interval, access });
        }
    
        bool Scope::IsWritingToSwapChain() const
        {
            return m_isWritingToSwapChainScope;
        }
    
        bool Scope::IsRequestingSwapChain() const
        {
            return m_isRequestingSwapChainDrawable;
        }
    }
}
