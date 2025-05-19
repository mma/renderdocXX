


#include "vk_dependency.h"
#include "../vk_debug.h"
#include "../vk_replay.h"
#include "../api/replay/rdcstr.h"
#include "strings/string_utils.h"


VulkanDepEngine::VulkanDepEngine(WrappedVulkan *core)
{
  m_pCore = core;

  m_pInputSDFile = NULL;

  m_DepDescriptorSetInfos.clear();

  m_DepCmdBufInfos.clear();

  m_SubmitOrder.clear();

  m_SubmissionIdxs.clear();

  m_AllKeyImages.clear();

  m_AllDepPassInfos.clear();
  m_NormalizedDepPassInfos.clear();
}

VulkanDepEngine::~VulkanDepEngine()
{

}

void VulkanDepEngine::ExtractDepInfos()
{
  // get structured file
  m_pInputSDFile = m_pCore->GetStructuredFile();
  RDCLOG("Dependency: get sturctured file done");

  // analyse chunks
  AnalyseChunks();
  RDCLOG("Dependency: analyse chunks done");

  // analyse dep info
  // for record submission idx info, call AnalyseDepInfo() in AnalyseOneChunk()
  //AnalyseDepInfo();
  //RDCLOG("Dependency: analyse dependency info done");

  // normalize
  Normalize();
  RDCLOG("Dependency: normalize done");
}

void VulkanDepEngine::AnalyseChunks()
{
  // only need to analyse chunks after CaptureBegin
  chunk_index idx = 0;
  for(; idx < m_pInputSDFile->chunks.size(); idx++)
  {
    if(IsInitialContentsChunk(m_pInputSDFile->chunks[idx]->metadata.chunkID))
      break;
  }

  for(; idx < m_pInputSDFile->chunks.size(); idx++)
  {
    RDCLOG("Dependency: idx %d", idx);
    AnalyseOneChunk(idx, m_pInputSDFile->chunks[idx]);
  }
}

void VulkanDepEngine::AnalyseOneChunk(chunk_index chunkIndex, SDChunk* pChunk)
{
  // for begin/end command buffer
  if (IsBeginCommandBufferChunk(pChunk->metadata.chunkID))
  {
    ResourceId cmdBufferId = ResourceId(pChunk->GetChild(0)->data.basic.u);

    m_DepCmdBufInfos.emplace(cmdBufferId, VulkanDepCommandBufferInfo(cmdBufferId, chunkIndex));
  }
  else if (IsEndCommandBufferChunk(pChunk->metadata.chunkID))
  {
    ResourceId cmdBufferId = ResourceId(pChunk->GetChild(0)->data.basic.u);

    m_DepCmdBufInfos[cmdBufferId].m_ciEnd = chunkIndex;
  }

  // for begin/end render pass
  else if (IsBeginRenderPassChunk(pChunk->metadata.chunkID))
  {
    ResourceId cmdBufferId = pChunk->GetChild(0)->data.basic.u;

    ResourceId renderPassId = ResourceId(pChunk->GetChild(1)->GetChild(1)->name == "pNextType"
                        ? pChunk->GetChild(1)->GetChild(3)->data.basic.u
                        : pChunk->GetChild(1)->GetChild(2)->data.basic.u);

    ResourceId frameBufferId = ResourceId(pChunk->GetChild(1)->GetChild(1)->name == "pNextType"
                        ? pChunk->GetChild(1)->GetChild(4)->data.basic.u
                        : pChunk->GetChild(1)->GetChild(3)->data.basic.u);

    VulkanDepPassInfo depPassInfo(vivo::PASS_TYPE_RENDERPASS, chunkIndex);
    depPassInfo.m_RenderPassId = renderPassId;
    depPassInfo.m_FrameBufferId = frameBufferId;

    ResourceId liveRenderPass = m_pCore->GetReplay()->GetLiveID(renderPassId);
    auto rpInfo = m_pCore->m_CreationInfo.m_RenderPass[liveRenderPass];

    ResourceId liveFrameBuffer = m_pCore->GetReplay()->GetLiveID(frameBufferId);
    auto fbInfo = m_pCore->m_CreationInfo.m_Framebuffer[liveFrameBuffer];

    RDCASSERT(rpInfo.attachments.size() == fbInfo.attachments.size());

    for (size_t i = 0; i < rpInfo.attachments.size(); i++)
    {
      auto rpAttachment = rpInfo.attachments[i];
      auto fbAttachment = fbInfo.attachments[i];

      auto imgViewInfo = m_pCore->m_CreationInfo.m_ImageView[fbAttachment.createdView];
      auto oriImageId = m_pCore->GetResourceManager()->GetOriginalID(imgViewInfo.image);

      VulkanDepOutputInfo depOutputInfo;
      depOutputInfo. m_Format = imgViewInfo.format;

      auto imgInfo = m_pCore->m_CreationInfo.m_Image[imgViewInfo.image];

      depOutputInfo.m_Width = imgInfo.extent.width;
      depOutputInfo.m_Height = imgInfo.extent.height;
      depOutputInfo.m_Depth = imgInfo.extent.depth;

      rdcarray<uint32_t> subresources =
          GetSubresources(oriImageId, imgViewInfo.range.baseMipLevel, imgViewInfo.range.levelCount,
                          imgViewInfo.range.baseArrayLayer, imgViewInfo.range.layerCount);
      depOutputInfo.m_ImageInfo = vivo::DepImageInfo(oriImageId, subresources);

      depOutputInfo.m_LoadOp = (VkAttachmentLoadOp)rpAttachment.loadOp;
      depOutputInfo.m_StoreOp = (VkAttachmentStoreOp)rpAttachment.storeOp;

      depOutputInfo.m_StencilLoadOp = (VkAttachmentLoadOp)rpAttachment.stencilLoadOp;
      depOutputInfo.m_StencilStoreOp = (VkAttachmentStoreOp)rpAttachment.stencilStoreOp;

      depPassInfo.m_OutputInfos.push_back(depOutputInfo);
    }

    m_DepCmdBufInfos[cmdBufferId].m_DepPassInfos.emplace(chunkIndex, depPassInfo);
  }
  else if (IsEndRenderPassChunk(pChunk->metadata.chunkID))
  {
    ResourceId cmdBufferId = ResourceId(pChunk->GetChild(0)->data.basic.u);

    auto iterRenderPassInfo = m_DepCmdBufInfos[cmdBufferId].m_DepPassInfos.rbegin();
    iterRenderPassInfo->second.m_ciEnd = chunkIndex;
  }

  // for descriptor sets
  else if (IsUpdateDescriptorSetsChunk(pChunk->metadata.chunkID))
  {
    // write
    auto pDescriptorWriteCount = pChunk->GetChild(1);
    auto pDescriptorWrites = pChunk->GetChild(2);
    for (uint64_t i = 0; i < pDescriptorWriteCount->data.basic.u; i++)
    {
      auto pDescriptorWrite = pDescriptorWrites->GetChild(i);
      auto pDstSet = pDescriptorWrite->GetChild(1)->name == "pNextType"
                         ? pDescriptorWrite->GetChild(3)
                         : pDescriptorWrite->GetChild(2);
      auto pDstBinding = pDescriptorWrite->GetChild(1)->name == "pNextType"
                                  ? pDescriptorWrite->GetChild(4)
                                  : pDescriptorWrite->GetChild(3);
      auto pDescriptorCount = pDescriptorWrite->GetChild(1)->name == "pNextType"
                                  ? pDescriptorWrite->GetChild(6)
                                  : pDescriptorWrite->GetChild(5);
      auto pDescriptorType = pDescriptorWrite->GetChild(1)->name == "pNextType"
                                 ? pDescriptorWrite->GetChild(7)
                                 : pDescriptorWrite->GetChild(6);

      ResourceId dstSetId = m_pCore->GetResourceManager()->GetOriginalID(pDstSet->data.basic.u);
      m_DepDescriptorSetInfos[dstSetId].m_DescriptorSetId = dstSetId;

      if (pDescriptorType->data.str == "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE" ||
          pDescriptorType->data.str == "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE" ||
          pDescriptorType->data.str == "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT")
      {
        auto pImageInfo = pDescriptorWrite->GetChild(1)->name == "pNextType"
                              ? pDescriptorWrite->GetChild(8)
                              : pDescriptorWrite->GetChild(7);
        for (uint64_t j = 0; j < pDescriptorCount->data.basic.u; j++)
        {
          auto oiImageView = pImageInfo->GetChild(j)->GetChild(1)->data.basic.u;
          ResourceId liveImageView = m_pCore->GetReplay()->GetLiveID(ResourceId(oiImageView));
          auto  createInfo = m_pCore->m_CreationInfo.m_ImageView[liveImageView];

          ResourceId oriImageId = m_pCore->GetResourceManager()->GetOriginalID(liveImageView);

          rdcarray<uint32_t> subresources;
          subresources =
              GetSubresources(oriImageId, createInfo.range.baseMipLevel, createInfo.range.levelCount,
                              createInfo.range.baseArrayLayer, createInfo.range.layerCount);

          if (pDescriptorType->data.str == "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE" ||
              pDescriptorType->data.str == "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE")
          {
            m_DepDescriptorSetInfos[dstSetId].m_ReadImages.emplace(uint32_t(pDstBinding->data.basic.u + j), vivo::DepImageInfo(oriImageId, subresources));
          }

          if (pDescriptorType->data.str == "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE" ||
              pDescriptorType->data.str == "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT")
          {
             m_DepDescriptorSetInfos[dstSetId].m_WriteImages.emplace(uint32_t(pDstBinding->data.basic.u + j), vivo::DepImageInfo(oriImageId, subresources));
          }
        }
      }
      else if (pDescriptorType->data.str == "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER")
      {
        auto pImageInfo = pDescriptorWrite->GetChild(1)->name == "pNextType"
                              ? pDescriptorWrite->GetChild(8)
                              : pDescriptorWrite->GetChild(7);
        for (uint64_t j = 0; j < pDescriptorCount->data.basic.u; j++)
        {
          auto oiImageView = pImageInfo->GetChild(j)->GetChild(1)->data.basic.u;
          ResourceId liveImageView = m_pCore->GetReplay()->GetLiveID(ResourceId(oiImageView));
          auto  createInfo = m_pCore->m_CreationInfo.m_ImageView[liveImageView];

          ResourceId oriImageId = m_pCore->GetResourceManager()->GetOriginalID(liveImageView);

          rdcarray<uint32_t> subresources;
          subresources =
              GetSubresources(oriImageId, createInfo.range.baseMipLevel, createInfo.range.levelCount,
                              createInfo.range.baseArrayLayer, createInfo.range.layerCount);

          m_DepDescriptorSetInfos[dstSetId].m_ReadImages.emplace(uint32_t(pDstBinding->data.basic.u + j), vivo::DepImageInfo(oriImageId, subresources));
        }
      }
      else
      {
        continue;
      }
    }

    // copy, need more test
    auto copyCount = pChunk->GetChild(3)->data.basic.u;
    RDCASSERT(!copyCount);
  }
  else if (IsInitialContentsChunk(pChunk->metadata.chunkID))
  {
    if(pChunk->GetChild(0)->data.str == "eResDescriptorSet")
    {
      ResourceId descriptorSetId = ResourceId(pChunk->GetChild(1)->data.basic.u);
      m_DepDescriptorSetInfos[descriptorSetId].m_DescriptorSetId = descriptorSetId;
      m_DepDescriptorSetInfos[descriptorSetId].m_ReadImages.clear();
      m_DepDescriptorSetInfos[descriptorSetId].m_WriteImages.clear();

      auto pBindings = pChunk->GetChild(2);
      for (size_t i = 0; i < pBindings->NumChildren(); i++)
      {
        auto pBinding = pBindings->GetChild(i);

        if (pBinding->GetChild(0)->data.str == "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE" ||
            pBinding->GetChild(0)->data.str == "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE" ||
            pBinding->GetChild(0)->data.str == "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT")
        {
          ResourceId liveImageView = m_pCore->GetReplay()->GetLiveID(ResourceId(pBinding->GetChild(1)->data.basic.u));
          auto  viewCreateInfo = m_pCore->m_CreationInfo.m_ImageView[liveImageView];
          auto imageCreateInfo = m_pCore->m_CreationInfo.m_Image[viewCreateInfo.image];

          ResourceId oriImageId = m_pCore->GetResourceManager()->GetOriginalID(viewCreateInfo.image);

          rdcarray<uint32_t> subresources;
          subresources = GetSubresources(
              oriImageId, viewCreateInfo.range.baseMipLevel, viewCreateInfo.range.levelCount,
              viewCreateInfo.range.baseArrayLayer, viewCreateInfo.range.layerCount);

          if (pBinding->GetChild(0)->data.str == "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE" ||
              pBinding->GetChild(0)->data.str == "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE")
          {
            m_DepDescriptorSetInfos[descriptorSetId].m_ReadImages.emplace(uint32_t(i), vivo::DepImageInfo(oriImageId, subresources));
          }
          else if (pBinding->GetChild(0)->data.str == "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE" ||
              pBinding->GetChild(0)->data.str == "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT")
          {
            m_DepDescriptorSetInfos[descriptorSetId].m_WriteImages.emplace(uint32_t(i), vivo::DepImageInfo(oriImageId, subresources));
          }
        }
        else if (pBinding->GetChild(0)->data.str == "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER")
        {
          ResourceId liveImageView = m_pCore->GetReplay()->GetLiveID(ResourceId(pBinding->GetChild(2)->data.basic.u));
          auto  viewCreateInfo = m_pCore->m_CreationInfo.m_ImageView[liveImageView];
          auto imageCreateInfo = m_pCore->m_CreationInfo.m_Image[viewCreateInfo.image];

          ResourceId oriImageId = m_pCore->GetResourceManager()->GetOriginalID(viewCreateInfo.image);

          rdcarray<uint32_t> subresources;
          subresources = GetSubresources(
              oriImageId, viewCreateInfo.range.baseMipLevel, viewCreateInfo.range.levelCount,
              viewCreateInfo.range.baseArrayLayer, viewCreateInfo.range.layerCount);

          m_DepDescriptorSetInfos[descriptorSetId].m_ReadImages.emplace(uint32_t(i), vivo::DepImageInfo(oriImageId, subresources));
        }
        else
        {
          continue;
        }
      }
    }
  }
  else if (IsBindDescSetsChunk(pChunk->metadata.chunkID))
  {
    ResourceId cmdBufferId = ResourceId(pChunk->GetChild(0)->data.basic.u);

    auto firstSet = pChunk->GetChild(3)->data.basic.u;
    auto descriptorSetCount = pChunk->GetChild(4)->data.basic.u;
    auto pDescriptorSets = pChunk->GetChild(5);

    for (auto slot = firstSet; slot < firstSet + descriptorSetCount; slot++)
    {
      ResourceId descriptorSet = ResourceId(pDescriptorSets[firstSet].GetChild(slot)->data.basic.u);

      m_DepCmdBufInfos[cmdBufferId].m_CurDescriptorSets.emplace(uint32_t(slot), descriptorSet);
    }
  }
  else if (IsPushDescriptorSetChunk(pChunk->metadata.chunkID))
  {
    // need more test
    RDCASSERT(0);
  }

  // for draw and dispatch
  else if (IsDrawChunk(pChunk->metadata.chunkID))
  {
    ResourceId cmdBufferId = ResourceId(pChunk->GetChild(0)->data.basic.u);

    std::map<chunk_index, VulkanDepPassInfo>::reverse_iterator iter;
    if (!m_DepCmdBufInfos[cmdBufferId].m_DepPassInfos.size())
    {
      m_DepCmdBufInfos[cmdBufferId].m_DepPassInfos.emplace(chunkIndex, VulkanDepPassInfo());
    }
    iter = m_DepCmdBufInfos[cmdBufferId].m_DepPassInfos.rbegin();

    for (auto curDescriptorSets : m_DepCmdBufInfos[cmdBufferId].m_CurDescriptorSets)
    {
      for (auto readImage : m_DepDescriptorSetInfos[curDescriptorSets.second].m_ReadImages)
      {
        VulkanDepInputInfo depInputInfo;
        depInputInfo.m_ImageInfo = readImage.second;
        depInputInfo.m_BindSlot = readImage.first;
        depInputInfo.m_ciDraw = chunkIndex;
        depInputInfo.m_Usage = "";

        iter->second.m_InputInfos[chunkIndex].push_back(depInputInfo);
      }

      for (auto writeImage : m_DepDescriptorSetInfos[curDescriptorSets.second].m_WriteImages)
      {
        VulkanDepInputInfo depInputInfo;
        depInputInfo.m_ImageInfo = writeImage.second;
        depInputInfo.m_BindSlot = writeImage.first;
        depInputInfo.m_ciDraw = chunkIndex;
        depInputInfo.m_Usage = "";

        iter->second.m_InputInfos[chunkIndex].push_back(depInputInfo);
      }
    }
  }
  else if (IsDispatchChunk(pChunk->metadata.chunkID))
  {
    ResourceId cmdBufferId = ResourceId(pChunk->GetChild(0)->data.basic.u);

    m_DepCmdBufInfos[cmdBufferId].m_DepPassInfos.emplace(chunkIndex, VulkanDepPassInfo(vivo::PASS_TYPE_COMPUTE, chunkIndex, chunkIndex));

    auto iter = m_DepCmdBufInfos[cmdBufferId].m_DepPassInfos.rbegin();

    for (auto curDescriptorSets : m_DepCmdBufInfos[cmdBufferId].m_CurDescriptorSets)
    {
      for (auto readImage : m_DepDescriptorSetInfos[curDescriptorSets.second].m_ReadImages)
      {
        VulkanDepInputInfo depInputInfo;
        depInputInfo.m_ImageInfo = readImage.second;
        depInputInfo.m_BindSlot = readImage.first;
        depInputInfo.m_ciDraw = chunkIndex;
        depInputInfo.m_Usage = "";

        iter->second.m_InputInfos[chunkIndex].push_back(depInputInfo);
      }

      for (auto writeImage : m_DepDescriptorSetInfos[curDescriptorSets.second].m_WriteImages)
      {
        VulkanDepInputInfo depInputInfo;
        depInputInfo.m_ImageInfo = writeImage.second;
        depInputInfo.m_BindSlot = writeImage.first;
        depInputInfo.m_ciDraw = chunkIndex;
        depInputInfo.m_Usage = "";

        iter->second.m_InputInfos[chunkIndex].push_back(depInputInfo);
      }
    }
  }

  // for copy image
  else if (IsCopyImageChunk(pChunk->metadata.chunkID))
  {
    ResourceId cmdBufferId = ResourceId(pChunk->GetChild(0)->data.basic.u);

    ResourceId srcImgId = ResourceId(pChunk->GetChild(1)->data.basic.u);
    ResourceId dstImgId = ResourceId(pChunk->GetChild(3)->data.basic.u);

    rdcarray<uint32_t> srcSubresources = GetSubresources(
        srcImgId, (uint32_t)pChunk->GetChild(6)->GetChild(0)->GetChild(1)->data.basic.u, 1,
        (uint32_t)pChunk->GetChild(6)->GetChild(0)->GetChild(2)->data.basic.u,
        (uint32_t)pChunk->GetChild(6)->GetChild(0)->GetChild(3)->data.basic.u);

    rdcarray<uint32_t> dstSubresources = GetSubresources(
        dstImgId, (uint32_t)pChunk->GetChild(6)->GetChild(2)->GetChild(1)->data.basic.u, 1,
        (uint32_t)pChunk->GetChild(6)->GetChild(2)->GetChild(2)->data.basic.u,
        (uint32_t)pChunk->GetChild(6)->GetChild(2)->GetChild(3)->data.basic.u);

    VulkanDepPassInfo depPassInfo(vivo::PASS_TYPE_COPY, chunkIndex, chunkIndex);

    VulkanDepInputInfo depInputInfo;
    depInputInfo.m_ImageInfo = vivo::DepImageInfo(srcImgId, srcSubresources);
    depInputInfo.m_BindSlot = 0;
    depInputInfo.m_ciDraw = 0;
    depInputInfo.m_Usage = "";

    depPassInfo.m_InputInfos[chunkIndex].push_back(depInputInfo);

    VulkanDepOutputInfo depOutputInfo;
    depOutputInfo.m_ImageInfo = vivo::DepImageInfo(dstImgId, dstSubresources);
    depOutputInfo.m_Format = m_pCore->m_CreationInfo.m_Image[dstImgId].format;
    depOutputInfo.m_Width = m_pCore->m_CreationInfo.m_Image[dstImgId].extent.width;
    depOutputInfo.m_Height = m_pCore->m_CreationInfo.m_Image[dstImgId].extent.height;
    depOutputInfo.m_Depth = m_pCore->m_CreationInfo.m_Image[dstImgId].extent.depth;

    depPassInfo.m_OutputInfos.push_back(depOutputInfo);

    m_DepCmdBufInfos[cmdBufferId].m_DepPassInfos.emplace(chunkIndex, depPassInfo);
  }
  else if (IsCopyImage2Chunk(pChunk->metadata.chunkID))
  {
    ResourceId cmdBufferId = ResourceId(pChunk->GetChild(0)->data.basic.u);

    ResourceId srcImgId;
    ResourceId dstImgId;
    uint32_t regionCount = 0;
    SDObject *pRegions = NULL;
    if (pChunk->GetChild(1)->GetChild(1)->name == "pNextType")
    {
      srcImgId = ResourceId(pChunk->GetChild(1)->GetChild(3)->data.basic.u);
      dstImgId = ResourceId(pChunk->GetChild(1)->GetChild(5)->data.basic.u);
      regionCount = (uint32_t)pChunk->GetChild(1)->GetChild(7)->data.basic.u;
      pRegions = pChunk->GetChild(1)->GetChild(8);
    }
    else
    {
      srcImgId = ResourceId(pChunk->GetChild(1)->GetChild(2)->data.basic.u);
      dstImgId = ResourceId(pChunk->GetChild(1)->GetChild(4)->data.basic.u);
      regionCount = (uint32_t)pChunk->GetChild(1)->GetChild(6)->data.basic.u;
      pRegions = pChunk->GetChild(1)->GetChild(7);
    }

    rdcarray<uint32_t> srcSubresources;
    rdcarray<uint32_t> dstSubresources;
    for (uint32_t i = 0; i < regionCount; i++)
    {
      if (pRegions[i].GetChild(1)->name == "pNextType")
      {
        srcSubresources = GetSubresources(
            srcImgId, (uint32_t)pRegions[i].GetChild(1)->GetChild(3)->GetChild(1)->data.basic.u, 1,
            (uint32_t)pRegions[i].GetChild(1)->GetChild(3)->GetChild(2)->data.basic.u,
            (uint32_t)pRegions[i].GetChild(1)->GetChild(3)->GetChild(3)->data.basic.u);

        dstSubresources = GetSubresources(
            dstImgId, (uint32_t)pRegions[i].GetChild(1)->GetChild(5)->GetChild(1)->data.basic.u, 1,
            (uint32_t)pRegions[i].GetChild(1)->GetChild(5)->GetChild(2)->data.basic.u,
            (uint32_t)pRegions[i].GetChild(1)->GetChild(5)->GetChild(3)->data.basic.u);
      }
      else
      {
        srcSubresources = GetSubresources(
            srcImgId, (uint32_t)pRegions[i].GetChild(1)->GetChild(2)->GetChild(1)->data.basic.u, 1,
            (uint32_t)pRegions[i].GetChild(1)->GetChild(2)->GetChild(2)->data.basic.u,
            (uint32_t)pRegions[i].GetChild(1)->GetChild(2)->GetChild(3)->data.basic.u);

        dstSubresources = GetSubresources(
            dstImgId, (uint32_t)pRegions[i].GetChild(1)->GetChild(4)->GetChild(1)->data.basic.u, 1,
            (uint32_t)pRegions[i].GetChild(1)->GetChild(4)->GetChild(2)->data.basic.u,
            (uint32_t)pRegions[i].GetChild(1)->GetChild(4)->GetChild(3)->data.basic.u);
      }
    }

    VulkanDepPassInfo depPassInfo(vivo::PASS_TYPE_COPY, chunkIndex, chunkIndex);

    VulkanDepInputInfo depInputInfo;
    depInputInfo.m_ImageInfo = vivo::DepImageInfo(srcImgId, srcSubresources);
    depInputInfo.m_BindSlot = 0;
    depInputInfo.m_ciDraw = 0;
    depInputInfo.m_Usage = "";

    depPassInfo.m_InputInfos[chunkIndex].push_back(depInputInfo);

    VulkanDepOutputInfo depOutputInfo;
    depOutputInfo.m_ImageInfo = vivo::DepImageInfo(dstImgId, dstSubresources);
    depOutputInfo.m_Format = m_pCore->m_CreationInfo.m_Image[dstImgId].format;
    depOutputInfo.m_Width = m_pCore->m_CreationInfo.m_Image[dstImgId].extent.width;
    depOutputInfo.m_Height = m_pCore->m_CreationInfo.m_Image[dstImgId].extent.height;
    depOutputInfo.m_Depth = m_pCore->m_CreationInfo.m_Image[dstImgId].extent.depth;

    depPassInfo.m_OutputInfos.push_back(depOutputInfo);

    m_DepCmdBufInfos[cmdBufferId].m_DepPassInfos.emplace(chunkIndex, depPassInfo);
  }

  // for execute commands
  else if (IsExecuteCommandsChunk(pChunk->metadata.chunkID))
  {
    ResourceId cmdBufferId = ResourceId(pChunk->GetChild(0)->data.basic.u);
    auto iter = m_DepCmdBufInfos[cmdBufferId].m_DepPassInfos.rbegin();

    auto commandBufferCount = uint32_t(pChunk->GetChild(1)->data.basic.u);
    auto pCommandBuffers = pChunk->GetChild(2);

    for (uint32_t i = 0; i < commandBufferCount; i++)
    {
      auto secCmdBufferId = ResourceId(pCommandBuffers->GetChild(i)->data.basic.u);

      for (auto depPassInfo : m_DepCmdBufInfos[secCmdBufferId].m_DepPassInfos)
      {
        for(auto vaule : depPassInfo.second.m_InputInfos)
            iter->second.m_InputInfos.emplace(vaule);
      }

      m_DepCmdBufInfos.erase(secCmdBufferId);
    }
  }

  // for submit
  else if(IsQueueSubmitChunk(pChunk->metadata.chunkID))
  {
    auto submitCount = pChunk->GetChild(1)->data.basic.u;
    auto pSubmits = pChunk->GetChild(2);

    for (uint64_t i = 0; i < submitCount; i++)
    {
      auto commandBufferCount = pSubmits->GetChild(i)->GetChild(1)->name == "pNextType"
                                    ? pSubmits->GetChild(i)->GetChild(6)->data.basic.u
                                    : pSubmits->GetChild(i)->GetChild(5)->data.basic.u;

      auto pCommandBuffers = pSubmits->GetChild(i)->GetChild(1)->name == "pNextType"
                                 ? pSubmits->GetChild(i)->GetChild(7)
                                 : pSubmits->GetChild(i)->GetChild(6);

      for (uint64_t j = 0; j < commandBufferCount; j++)
      {
        if(m_SubmissionIdxs.find(ResourceId(pCommandBuffers->GetChild(j)->data.basic.u)) !=
           m_SubmissionIdxs.end())
          m_SubmissionIdxs[ResourceId(pCommandBuffers->GetChild(j)->data.basic.u)]++;
        else
          m_SubmissionIdxs[ResourceId(pCommandBuffers->GetChild(j)->data.basic.u)] = 0;

        m_SubmitOrder.push_back(ResourceId(pCommandBuffers->GetChild(j)->data.basic.u));
      }
    }

    AnalyseDepInfo();
  }
  else if(IsQueueSubmit2Chunk(pChunk->metadata.chunkID))
  {
    auto submitCount = pChunk->GetChild(1)->data.basic.u;
    auto pSubmits = pChunk->GetChild(2);

    for (uint64_t i = 0; i < submitCount; i++)
    {
      auto commandBufferInfoCount = pSubmits->GetChild(i)->GetChild(1)->name == "pNextType"
                                        ? pSubmits->GetChild(i)->GetChild(6)->data.basic.u
                                        : pSubmits->GetChild(i)->GetChild(5)->data.basic.u;

      auto pCommandBufferInfos = pSubmits->GetChild(i)->GetChild(1)->name == "pNextType"
                                     ? pSubmits->GetChild(i)->GetChild(7)
                                     : pSubmits->GetChild(i)->GetChild(6);

      for (uint64_t j = 0; j < commandBufferInfoCount; j++)
      {
        auto commandBuffer = pCommandBufferInfos->GetChild(j)->GetChild(1)->name == "pNextType"
                                 ? pCommandBufferInfos->GetChild(j)->GetChild(3)->data.basic.u
                                 : pCommandBufferInfos->GetChild(j)->GetChild(2)->data.basic.u;

        if(m_SubmissionIdxs.find(ResourceId(commandBuffer)) != m_SubmissionIdxs.end())
          m_SubmissionIdxs[ResourceId(commandBuffer)]++;
        else
          m_SubmissionIdxs[ResourceId(commandBuffer)] = 0;

        m_SubmitOrder.push_back(ResourceId(commandBuffer));
      }
    }

    AnalyseDepInfo();
  }
}

void VulkanDepEngine::AnalyseDepInfo()
{
  for (auto cmdBufId : m_SubmitOrder)
  {
    for (auto passInfos : m_DepCmdBufInfos[cmdBufId].m_DepPassInfos)
    {
      VulkanDepPassInfo depPassInfo;
      depPassInfo.m_PassType = passInfos.second.m_PassType;
      depPassInfo.m_ciStart = passInfos.second.m_ciStart;
      depPassInfo.m_ciEnd = passInfos.second.m_ciEnd;
      depPassInfo.m_CmdBufId = cmdBufId;
      depPassInfo.m_SubmissionIdx = m_SubmissionIdxs[cmdBufId];

      depPassInfo.m_RenderPassId = passInfos.second.m_RenderPassId;
      depPassInfo.m_FrameBufferId = passInfos.second.m_FrameBufferId;

      for (auto inputInfos : passInfos.second.m_InputInfos)
      {
        bool bKeyImage = false;
        rdcarray<VulkanDepInputInfo> vecInputInfos;
        for (auto inputInfo : inputInfos.second)
        {
          rdcarray<uint32_t> keyIndexes;
          if (IsKeyImages(inputInfo.m_ImageInfo.m_ImageID, inputInfo.m_ImageInfo.m_Subresources, keyIndexes))
          {
            vecInputInfos.push_back(inputInfo);
            bKeyImage = true;
          }
        }

        if(bKeyImage)
          depPassInfo.m_InputInfos.emplace(inputInfos.first, vecInputInfos);
      }

      for (auto outputInfos : passInfos.second.m_OutputInfos)
      {
        RecordKeyImages(outputInfos.m_ImageInfo.m_ImageID, outputInfos.m_ImageInfo.m_Subresources);

        depPassInfo.m_OutputInfos.push_back(outputInfos);
      }

      m_AllDepPassInfos.push_back(depPassInfo);
    }
  }

  m_SubmitOrder.clear();
}

void VulkanDepEngine::RecordKeyImages(ResourceId imgId, rdcarray<uint32_t> subresources)
{
  if (m_AllKeyImages.find(imgId) != m_AllKeyImages.end())
  {
    m_AllKeyImages[imgId].insert(m_AllKeyImages[imgId].size(), subresources);
  }
  else
  {
    m_AllKeyImages.emplace(imgId, subresources);
  }
}

bool VulkanDepEngine::IsKeyImages(ResourceId imgId, rdcarray<uint32_t> subresources, rdcarray<uint32_t>& keyIndexes)
{
  bool ret = false;

  if(m_AllKeyImages.find(imgId) == m_AllKeyImages.end())
    return ret;

  uint32_t index = 0;
  for (auto subresource : subresources)
  {
    if (std::find(m_AllKeyImages[imgId].begin(), m_AllKeyImages[imgId].end(), subresource) != m_AllKeyImages[imgId].end())
    {
      ret = true;
      keyIndexes.push_back(index);
    }

    index++;
  }

  return ret;
}

rdcarray<uint32_t> VulkanDepEngine::GetSubresources(ResourceId imgId, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount)
{
  rdcarray<uint32_t> subresources;

  auto arrayLayers = m_pCore->m_CreationInfo.m_Image[imgId].arrayLayers;

  for (uint32_t m = 0; m < levelCount; m++)
  {
    for (uint32_t a = 0; a < layerCount; a++)
    {
      subresources.push_back((baseMipLevel + m) * arrayLayers + (baseArrayLayer + a));
    }
  }

  return subresources;
}

void VulkanDepEngine::Normalize()
{
  std::map<uint32_t, uint32_t> chunkIndexEids;
  auto maxEid = m_pCore->GetMaxEID();
  for (uint32_t i = 0; i < maxEid; i++)
  {
    auto event = m_pCore->GetEvent(i);
    chunkIndexEids.emplace(event.chunkIndex, event.eventId);
  }

  for (auto value0 : m_AllDepPassInfos)
  {
    vivo::DepPassInfo passInfo;

    passInfo.m_PassIdx = 0;
    passInfo.m_StartEid = chunkIndexEids[value0.m_ciStart];
    passInfo.m_EndEid = chunkIndexEids[value0.m_ciEnd];
    passInfo.m_PassType = value0.m_PassType;
    passInfo.m_CmdBuf = value0.m_CmdBufId;
    passInfo.m_SubmissionIdx = value0.m_SubmissionIdx;
    passInfo.m_RP = value0.m_RenderPassId;
    passInfo.m_FBO = value0.m_FrameBufferId;

    for (auto value1 : value0.m_InputInfos)
    {
      passInfo.m_DrawEid.push_back(chunkIndexEids[value1.first]);

      for (auto value2 : value1.second)
      {
        vivo::DepInputInfo inputInfo;
        inputInfo.m_ImageInfo = value2.m_ImageInfo;
        inputInfo.m_BindSlot = value2.m_BindSlot;
        inputInfo.m_DrawEid = chunkIndexEids[value2.m_ciDraw];
        inputInfo.m_Usage = value2.m_Usage;

        passInfo.m_InputInfos.push_back(inputInfo);
      }
    }

    for (auto value1 : value0.m_OutputInfos)
    {
      vivo::DepOutputInfo outputInfo;
      outputInfo.m_ImageInfo = value1.m_ImageInfo;
      outputInfo.m_Format = MakeResourceFormat(value1.m_Format);
      outputInfo.m_Width = value1.m_Width;
      outputInfo.m_Height= value1.m_Height;
      outputInfo.m_Depth= value1.m_Depth;
      outputInfo.m_LoadOp= vivo::DepLoadOp(value1.m_LoadOp);
      outputInfo.m_StoreOp= vivo::DepStoreOp(value1.m_StoreOp);
      outputInfo.m_StencilLoadOp= vivo::DepLoadOp(value1.m_StencilLoadOp);
      outputInfo.m_StencilStoreOp= vivo::DepStoreOp(value1.m_StencilStoreOp);

      passInfo.m_OutputInfos.push_back(outputInfo);
    }

    if(!value0.m_OutputInfos.size())
      continue;

    m_NormalizedDepPassInfos.push_back(passInfo);
  }
}


