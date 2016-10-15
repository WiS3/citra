// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "video_core/debug_utils/debug_utils.h"
#include "video_core/renderer_opengl/gl_rasterizer_cache.h"

namespace Filtering {

enum class FilteringTypes {
    NONE = 0,
    XBRZ = 1,
};

// Checks if scaling is enabled
bool isScalingEnabled();

// Returns the currently configured scaling size
int getScaling();

// Returns the currently configured scaling type
Filtering::FilteringTypes getScalingType();

// Filters a texture using the specified texture
void filterTexture(Pica::DebugUtils::TextureInfo tex_info, unsigned int* fromBuffer,
                   unsigned int* toBuffer);

// Returns the scaled texture size (width * height) of this texture
int getScaledTextureSize(Pica::Regs::TextureFormat format, int width, int height);
}
