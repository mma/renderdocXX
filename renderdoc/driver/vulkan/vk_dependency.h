#pragma once

#include "../vk_core.h"
#include "serialise/rdcfile.h"


// the chunk index in SDFile::chunks, which will be used frequently
using chunk_index = uint32_t;
// the original resource id ( ResourceId::id ) in .rdc or .xml
using original_id = uint64_t;

class VulkanDepInputInfo
{
public:
  VulkanDepInputInfo() = default;

  ~VulkanDepInputInfo() = default;

public:
  vivo::DepImageInfo m_ImageInfo;

  uint32_t m_BindSlot;

  chunk_index m_ciDraw;

  rdcstr m_Usage;
};

class VulkanDepOutputInfo
{
public:
  VulkanDepOutputInfo() = default;
  ~VulkanDepOutputInfo() = default;

public:
  vivo::DepImageInfo m_ImageInfo;

  VkFormat m_Format;

  uint32_t m_Width;
  uint32_t m_Height;
  uint32_t m_Depth;

  VkAttachmentLoadOp m_LoadOp;
  VkAttachmentStoreOp m_StoreOp;
  VkAttachmentLoadOp m_StencilLoadOp;
  VkAttachmentStoreOp m_StencilStoreOp;
};

class VulkanDepPassInfo
{
public:
  VulkanDepPassInfo() = default;

  VulkanDepPassInfo(vivo::DepPassType passType, chunk_index ciStart, chunk_index ciEnd = 0) : m_PassType(passType), m_ciStart(ciStart), m_ciEnd(ciEnd)
  {
    m_CmdBufId = 0;
    m_SubmissionIdx = 0;

    m_InputInfos.clear();
    m_OutputInfos.clear();

    m_RenderPassId = 0;
    m_FrameBufferId = 0;
  }

  ~VulkanDepPassInfo() = default;

public:
  vivo::DepPassType m_PassType;

  chunk_index m_ciStart;
  chunk_index m_ciEnd;

  ResourceId m_CmdBufId;
  uint32_t m_SubmissionIdx;

  // chunk index of draw, dispatch and copy, etc.
  std::map<chunk_index, rdcarray<VulkanDepInputInfo>> m_InputInfos;
  rdcarray<VulkanDepOutputInfo> m_OutputInfos;

  // only for render pass
  ResourceId m_RenderPassId;
  ResourceId m_FrameBufferId;
};

class VulkanDepDescriptorSetInfo
{
public:
  VulkanDepDescriptorSetInfo() = default;
  ~VulkanDepDescriptorSetInfo() = default;

  VulkanDepDescriptorSetInfo(ResourceId descriptorSetId)
      : m_DescriptorSetId(descriptorSetId)
  {
  }

public:
  ResourceId m_DescriptorSetId;

  // slot, image
  std::map<uint32_t, vivo::DepImageInfo> m_ReadImages;
  std::map<uint32_t, vivo::DepImageInfo> m_WriteImages;
};

class VulkanDepCommandBufferInfo
{
public:
  VulkanDepCommandBufferInfo() = default;

  VulkanDepCommandBufferInfo(ResourceId commandBufferId, chunk_index ciStart) : m_CommandBufferId(commandBufferId), m_ciStart(ciStart)
  {
    m_ciEnd = 0;

    m_DepPassInfos.clear();
  }

  ~VulkanDepCommandBufferInfo() = default;

public:
  ResourceId m_CommandBufferId;

  chunk_index m_ciStart;
  chunk_index m_ciEnd;

  // slot, descriptor set
  std::map<uint32_t, ResourceId> m_CurDescriptorSets;

  // chunk index of vkCmdBeginRenderPass/.. or vkCmdDispatch/.. of vkCmdCopyImage, pass info
  std::map<chunk_index, VulkanDepPassInfo> m_DepPassInfos;
};

class VulkanDepEngine
{
public:
  VulkanDepEngine(WrappedVulkan *core);

  ~VulkanDepEngine();

  void ExtractDepInfos();

  rdcarray<vivo::DepPassInfo> GetDepPassInfos() { return m_NormalizedDepPassInfos; }

private:
  void AnalyseChunks();

  void AnalyseOneChunk(chunk_index chunkIndex, SDChunk *pChunk);

  void AnalyseDepInfo();

  void RecordKeyImages(ResourceId imgId, rdcarray<uint32_t> subresources);

  bool IsKeyImages(ResourceId imgId, rdcarray<uint32_t> subresources, rdcarray<uint32_t>& keyIndexes);

  rdcarray<uint32_t> GetSubresources(ResourceId imgId, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount);

  void Normalize();

private:
  WrappedVulkan *m_pCore;

  // the input structured file
  SDFile *m_pInputSDFile;

  std::map<ResourceId, VulkanDepDescriptorSetInfo> m_DepDescriptorSetInfos;

  std::map<ResourceId, VulkanDepCommandBufferInfo> m_DepCmdBufInfos;

  std::vector<ResourceId> m_SubmitOrder;

  std::map<ResourceId, uint32_t> m_SubmissionIdxs;

  // temp record
  std::map<ResourceId, rdcarray<uint32_t>> m_AllKeyImages; // key images: 1. as fb resource; 2. wrote by dispatch; 3. dst images copied from images mentioned above;

  rdcarray<VulkanDepPassInfo> m_AllDepPassInfos;
  rdcarray<vivo::DepPassInfo> m_NormalizedDepPassInfos;
};

inline bool IsCaptureBeginChunk(uint32_t chunkID)
{
  return chunkID == (uint32_t)SystemChunk::CaptureBegin;
}

inline bool IsCopyImageChunk(uint32_t chunkID)
{
  return chunkID == (uint32_t)VulkanChunk::vkCmdCopyImage;
}

inline bool IsCopyImage2Chunk(uint32_t chunkID)
{
  return chunkID == (uint32_t)VulkanChunk::vkCmdCopyImage2;
}

inline bool IsQueueSubmitChunk(uint32_t chunkID)
{
  return chunkID == (uint32_t)VulkanChunk::vkQueueSubmit;
}

inline bool IsQueueSubmit2Chunk(uint32_t chunkID)
{
  return chunkID == (uint32_t)VulkanChunk::vkQueueSubmit2;
}