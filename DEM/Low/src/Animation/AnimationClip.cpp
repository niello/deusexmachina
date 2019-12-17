#include "AnimationClip.h"

namespace DEM::Anim
{

}

/*
std::ifstream File(options.input_filename, std::ios_base::in | std::ios_base::binary);
if (File.is_open())
{
	const size_t buffer_size = GET_CLIP_SIZE_FROM_DEM_HEADER

	char* buffer = static_cast<char*>(allocator.allocate(buffer_size, alignof(acl::CompressedClip)));
	File.read(buffer, buffer_size);

	pClip = reinterpret_cast<const CompressedClip*>(buffer);
	ACL_ASSERT(pClip->is_valid(true).empty(), "Compressed clip is invalid");

	...

	allocator.deallocate(buffer, buffer_size);
}
*/