// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "video_core/debug_utils/debug_utils.h"
#include "video_core/renderer_opengl/gl_rasterizer_cache.h"

namespace Filtering {

// TODO: Dynamic config of this
// 1-6, 1 disables
const int scale = 6;

enum class FilteringTypes {
    XBR = 0,
};

// Checks if scaling is enabled
const bool isScalingEnabled();

// Filters a texture using the specified texture
void filterTexture(FilteringTypes type, Pica::DebugUtils::TextureInfo tex_info, unsigned int * fromBuffer, unsigned int * toBuffer);

// Returns the scaled texture size (width * height) of this texture
int getScaledTextureSize(Pica::Regs::TextureFormat format, int width, int height);

}
