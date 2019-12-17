#include "AnimationPlayer.h"

namespace DEM::Anim
{

}

/*
acl::ANSIAllocator        ACLAllocator;

//////////////////////////////////////////////////////////////////////////
// Allocates and constructs an instance of the decompression context
template<class DecompressionSettingsType>
inline DecompressionContext<DecompressionSettingsType>* make_decompression_context(IAllocator& allocator)
{
	return allocate_type<DecompressionContext<DecompressionSettingsType>>(allocator, &allocator);
}

DecompressionContext<DecompressionSettingsType>::release()

-------------------------

using namespace acl;
using namespace acl::uniformly_sampled;

DecompressionContext<DefaultDecompressionSettings> context;

//???only if DecompressionContext<DecompressionSettingsType>::is_dirty(const CompressedClip& clip)

context.initialize(*compressed_clip);
context.seek(sample_time, sample_rounding_policy::none);

context.decompress_bone(bone_index, &rotation, &translation, &scale);

-------------------------

Transform_32* transforms = new Transform_32[num_bones];
DefaultOutputWriter pose_writer(transforms, num_bones);
context.decompress_pose(pose_writer);
*/