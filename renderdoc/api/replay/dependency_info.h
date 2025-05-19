using namespace vivo
{
enum DepPassType
{
  PASS_TYPE_RENDERPASS = 0,
  PASS_TYPE_COMPUTE,
  PASS_TYPE_COPY,
};

enum DepLoadOp
{
  LOAD_OP_LOAD = 0,
  LOAD_OP_CLEAR = 1,
  LOAD_OP_DONT_CARE = 2,
  LOAD_OP_NONE_KHR = 1000400000
};

enum DepStoreOp
{
  STORE_OP_STORE = 0,
  STORE_OP_DONT_CARE = 1,
  STORE_OP_NONE = 1000301000,
};

struct DepImageInfo
{
  DepImageInfo() = default;
  DepImageInfo(ResourceId imgId, rdcarray<uint32_t> subresources)
        : m_ImageID(imgId), m_Subresources(subresources)
  {
  }

  bool operator==(const DepImageInfo& in) const
  {
    return m_ImageID == in.m_ImageID && m_Subresources == in.m_Subresources;
  }

  ResourceId m_ImageID;
  rdcarray<uint32_t> m_Subresources;
};

struct DepInputInfo
{
  bool operator==(const DepInputInfo& in) const
  {
    return m_ImageInfo == in.m_ImageInfo && m_BindSlot == in.m_BindSlot && m_DrawEid == in.m_DrawEid && m_Usage == in.m_Usage;
  }
  bool operator<(const DepInputInfo &in) const
  {
    return m_BindSlot < in.m_BindSlot;
  }

  DepImageInfo m_ImageInfo;

  uint32_t m_BindSlot;

  uint32_t m_DrawEid;
  
  rdcstr m_Usage;
};

struct DepOutputInfo
{
  bool operator==(const DepOutputInfo& in) const
  {
    return m_ImageInfo == in.m_ImageInfo && m_Format == in.m_Format && m_Width == in.m_Width &&
           m_Height == in.m_Height && m_Depth == in.m_Depth && m_LoadOp == in.m_LoadOp &&
           m_StoreOp == in.m_StoreOp && m_StencilLoadOp == in.m_StencilLoadOp &&
           m_StencilStoreOp == in.m_StencilStoreOp;
  }
  bool operator<(const DepOutputInfo &in) const
  {
    return false;
  }

  DepImageInfo m_ImageInfo;

  ResourceFormat m_Format;

  uint32_t m_Width;
  uint32_t m_Height;
  uint32_t m_Depth;

  DepLoadOp m_LoadOp;
  DepStoreOp m_StoreOp;
  DepLoadOp m_StencilLoadOp;
  DepStoreOp m_StencilStoreOp;
};

struct DepPassInfo
{
  DepPassInfo &operator=(const DepPassInfo &) = default;

  bool operator==(const DepPassInfo &in) const
  {
    if(m_PassIdx != in.m_PassIdx)
      return false;
    
    if(m_StartEid != in.m_StartEid)
      return false;

    if(m_EndEid != in.m_EndEid)
      return false;

    if(m_PassType != in.m_PassType)
      return false;

    if(m_CmdBuf != in.m_CmdBuf)
      return false;

    if(m_SubmissionIdx != in.m_SubmissionIdx)
      return false;

    if(m_RP != in.m_RP)
      return false;

    if(m_FBO != in.m_FBO)
      return false;

    if(m_DrawEid != in.m_DrawEid)
      return false;

    if(m_InputInfos != in.m_InputInfos)
      return false;

    if(m_OutputInfos != in.m_OutputInfos)
      return false;

    return true;
  }

  bool operator<(const DepPassInfo &in) const
  {
    return m_PassIdx < in.m_PassIdx;
  }

  /* for all pass */
  uint32_t m_PassIdx;

  uint32_t m_StartEid;
  uint32_t m_EndEid;

  DepPassType m_PassType;

  ResourceId m_CmdBuf;
  uint32_t m_SubmissionIdx;

  /* for render pass */
  ResourceId m_RP;
  ResourceId m_FBO;

  // input images
  rdcarray<uint32_t> m_DrawEid;
  rdcarray<DepInputInfo> m_InputInfos;

  // output images
  rdcarray<DepOutputInfo> m_OutputInfos;
};

}
DECLARE_REFLECTION_ENUM(vivo::DepPassType);
DECLARE_REFLECTION_ENUM(vivo::DepLoadOp);
DECLARE_REFLECTION_ENUM(vivo::DepStoreOp);

DECLARE_REFLECTION_STRUCT(vivo::DepImageInfo);
DECLARE_REFLECTION_STRUCT(vivo::DepInputInfo);
DECLARE_REFLECTION_STRUCT(vivo::DepOutputInfo);
DECLARE_REFLECTION_STRUCT(vivo::DepPassInfo);
