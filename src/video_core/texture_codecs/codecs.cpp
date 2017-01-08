#include <cstring>
#include <memory>
#include <utility>
#include "common/assert.h"
#include "common/swap.h"
#include "common/vectorize.h"
#include "video_core/texture_codecs/codecs.h"

// Note: The function layout is made with two purposes. First for the understanding
// of the algorithm and second, to help the compiler unfold the loop and simplify
// the moves to the best appropiate type in use. compiling for ivy-bridge-up
// will unfold the loop further and use AVX2

// @param read_size is the amount of bytes each pixel takes
template <int read_size>
inline void decode_morton(u8* morton_pointer, u8* matrix_pointer) {
    std::memcpy(matrix_pointer, morton_pointer, read_size * 2);
}

// @param read_size is the amount of bytes each pixel takes
template <int read_size>
inline void encode_morton(u8* morton_pointer, u8* matrix_pointer) {
    std::memcpy(morton_pointer, matrix_pointer, read_size * 2);
}

// finaly, we decode to cursors/encode to blocks, the corresponding data by
// moving the appropiate
// 02 03 -> encode/decode second
// -----
// 00 01 -> encode/decode first
template <void codec_function(u8*, u8*), int read_size>
inline void morton_block2x2(u8* morton_block, u8*& w1, u8*& w2) {
    codec_function(morton_block, w1);
    w1 += read_size * 2;
    codec_function(morton_block + read_size * 2, w2);
    w2 += read_size * 2;
}

// Again, we subdivide the 4x4 tiles and assign the each 2x2 subblock to the
// corresponding cursors.
//
// 10 11 | 14 15
// 08 09 | 12 13
// ------------
// 02 03 | 06 07
// 00 01 | 04 05
template <void codec_function(u8*, u8*), int read_size>
inline void morton_block4x4(u8* morton_block, u8** w1, u8** w2) {
    u8* tmp_block = morton_block;
    morton_block2x2<codec_function, read_size>(tmp_block, w1[0], w1[1]);
    tmp_block += read_size * 4;
    morton_block2x2<codec_function, read_size>(tmp_block, w1[0], w1[1]);
    tmp_block += read_size * 4;
    morton_block2x2<codec_function, read_size>(tmp_block, w2[0], w2[1]);
    tmp_block += read_size * 4;
    morton_block2x2<codec_function, read_size>(tmp_block, w2[0], w2[1]);
}

// We subdivide the 8x8 tiles and assign the each 4x4 subblock to the
// corresponding cursors.
//
// 42 43 46 47 | 58 59 62 63
// 40 41 44 45 | 56 57 60 61
// 34 35 38 39 | 50 51 54 55
// 32 33 36 37 | 48 49 52 53
// -----------------------
// 10 11 14 15 | 26 27 30 31
// 08 09 12 13 | 24 25 28 29
// 02 03 06 07 | 18 19 22 23
// 00 01 04 05 | 16 17 20 21
template <void codec_function(u8*, u8*), int read_size>
inline void morton_block8x8(u8* from, u8** cursors) {
    u8* tmp_block = from;
    morton_block4x4<codec_function, read_size>(tmp_block, &cursors[0], &cursors[2]);
    tmp_block += read_size * 16;
    morton_block4x4<codec_function, read_size>(tmp_block, &cursors[0], &cursors[2]);
    tmp_block += read_size * 16;
    morton_block4x4<codec_function, read_size>(tmp_block, &cursors[4], &cursors[6]);
    tmp_block += read_size * 16;
    morton_block4x4<codec_function, read_size>(tmp_block, &cursors[4], &cursors[6]);
}

template <int read_size>
inline void rewind_cursors(u8** cursors, u8* matrix_buffer, u32 width) {
    cursors[0] = matrix_buffer;
    cursors[1] = matrix_buffer - read_size * width;
    cursors[2] = matrix_buffer - read_size * 2 * width;
    cursors[3] = matrix_buffer - read_size * 3 * width;
    cursors[4] = matrix_buffer - read_size * 4 * width;
    cursors[5] = matrix_buffer - read_size * 5 * width;
    cursors[6] = matrix_buffer - read_size * 6 * width;
    cursors[7] = matrix_buffer - read_size * 7 * width;
}

// from video_cor/utils.h
// Images are split into 8x8 tiles. Each tile is composed of four 4x4 subtiles each
// of which is composed of four 2x2 subtiles each of which is composed of four texels.
// Each structure is embedded into the next-bigger one in a diagonal pattern, e.g.
// texels are laid out in a 2x2 subtile like this:
// 2 3
// 0 1
//
// The full 8x8 tile has the texels arranged like this:
//
// 42 43 46 47 58 59 62 63
// 40 41 44 45 56 57 60 61
// 34 35 38 39 50 51 54 55
// 32 33 36 37 48 49 52 53
// 10 11 14 15 26 27 30 31
// 08 09 12 13 24 25 28 29
// 02 03 06 07 18 19 22 23
// 00 01 04 05 16 17 20 21
//
// This pattern is what's called Z-order curve, or Morton order.
//
// The algorithm below processos z-ordered images block by block.
// reading/writting in 8 cursors which point to the start of each
// row of a normal width*height raw pixel image.
template <void codec_function(u8*, u8*), int read_size>
inline void morton(u8* morton_buffer, u8* matrix_buffer, u32 width, u32 height) {
    u32 x_blocks = (width / 8);
    u32 y_blocks = (height / 8);
    u8* block_pointer = morton_buffer;
    u8* cursors[8];
    u32 step = (8 * width) * read_size;
    matrix_buffer += read_size * (width * (height - 1));
    for (u32 y = 0; y != y_blocks; y++) {
        rewind_cursors<read_size>(cursors, matrix_buffer, width);
        VECTORIZE_NEXT for (u32 x = 0; x != x_blocks; x++) {
            morton_block8x8<codec_function, read_size>(block_pointer, cursors);
            block_pointer += 64 * read_size;
        }
        matrix_buffer -= step;
    }
}

namespace Pica {

namespace Encoders {

bool Morton(u8* matrix_buffer, u8* morton_buffer, u32 width, u32 height, u32 bytespp) {
    // Sanity checks
    ASSERT(morton_buffer != nullptr && matrix_buffer != nullptr);
    ASSERT(((u64)morton_buffer & 3) == 0);
    ASSERT(((u64)matrix_buffer & 3) == 0);
    ASSERT(width >= 8);
    ASSERT(height >= 8);
    ASSERT((width * height) % 64 == 0);
    // --------------------------------
    switch (bytespp) {
    case 1: {
        morton<&encode_morton<1>, 1>(morton_buffer, matrix_buffer, width, height);
        return true;
        break;
    }
    case 2: {
        morton<&encode_morton<2>, 2>(morton_buffer, matrix_buffer, width, height);
        return true;
        break;
    }
    case 3: {
        morton<&encode_morton<3>, 3>(morton_buffer, matrix_buffer, width, height);
        return true;
        break;
    }
    case 4: {
        morton<&encode_morton<4>, 4>(morton_buffer, matrix_buffer, width, height);
        return true;
        break;
    }
    default: {
        return false;
        break;
    }
    }
}

inline void rotate_left(u8*& target_buffer) {
    u32 tmp;
    std::memcpy(&tmp, target_buffer, sizeof(u32));
    tmp = (tmp >> 8) | (tmp << 24);
    std::memcpy(target_buffer, &tmp, sizeof(u32));
    target_buffer += 4;
}
void Depth(u8* target_buffer, u32 width, u32 height) {
    Common::map<u8, &rotate_left>(target_buffer, width * height);
}

} // Encoders

namespace Decoders {

bool Morton(u8* morton_buffer, u8* matrix_buffer, u32 width, u32 height, u32 bytespp) {
    // Sanity checks
    ASSERT(morton_buffer != nullptr && matrix_buffer != nullptr);
    ASSERT(((u64)morton_buffer & 3) == 0);
    ASSERT(((u64)matrix_buffer & 3) == 0);
    ASSERT(width >= 8);
    ASSERT(height >= 8);
    ASSERT((width * height) % 64 == 0);
    switch (bytespp) {
    case 1: {
        morton<&decode_morton<1>, 1>(morton_buffer, matrix_buffer, width, height);
        return true;
        break;
    }
    case 2: {
        morton<&decode_morton<2>, 2>(morton_buffer, matrix_buffer, width, height);
        return true;
        break;
    }
    case 3: {
        morton<&decode_morton<3>, 3>(morton_buffer, matrix_buffer, width, height);
        return true;
        break;
    }
    case 4: {
        morton<&decode_morton<4>, 4>(morton_buffer, matrix_buffer, width, height);
        return true;
        break;
    }
    default: {
        return false;
        break;
    }
    }
}

//@note movbe intrinsics?
inline void big_u32(u8*& target_buffer) {
    u32 tmp;
    std::memcpy(&tmp, target_buffer, sizeof(u32));
    tmp = Common::swap32(tmp);
    std::memcpy(target_buffer, &tmp, sizeof(u32));
    target_buffer += 4;
}
void BigEndian(u8* target_buffer, u32 width, u32 height) {
    Common::map<u8, &big_u32>(target_buffer, width * height);
}

inline void rotate_right(u8*& target_buffer) {
    u32 tmp;
    std::memcpy(&tmp, target_buffer, sizeof(u32));
    tmp = (tmp >> 24) | (tmp << 8);
    std::memcpy(target_buffer, &tmp, sizeof(u32));
    target_buffer += 4;
}
void Depth(u8* target_buffer, u32 width, u32 height) {
    Common::map<u8, &rotate_right>(target_buffer, width * height);
}

inline void color_i8(u8*& read_cursor, u8*& write_cursor) {
    read_cursor -= 1;
    write_cursor -= 4;
    u32 tmp = 0;
    u8 tmp2;
    std::memcpy(&tmp2, read_cursor, sizeof(u8));
    tmp = tmp2 & 0x000000FF;
    tmp = (tmp << 16) | (tmp << 8) | tmp | 0xFF000000;
    std::memcpy(write_cursor, &tmp, sizeof(u32));
}
void I8(u8* target_buffer, u32 width, u32 height) {
    u8* read_cursor = target_buffer + (width * height);
    u8* write_cursor = target_buffer + (width * height * 4);
    Common::unfold<u8, &color_i8>(read_cursor, write_cursor, width * height);
}

inline void color_a8(u8*& read_cursor, u8*& write_cursor) {
    read_cursor -= 1;
    write_cursor -= 4;
    u32 tmp = 0;
    u8 tmp2;
    std::memcpy(&tmp2, read_cursor, sizeof(u8));
    tmp = tmp2 & 0x000000FF;
    tmp = tmp << 24;
    std::memcpy(write_cursor, &tmp, sizeof(u32));
}

void A8(u8* target_buffer, u32 width, u32 height) {
    u8* read_cursor = target_buffer + (width * height);
    u8* write_cursor = target_buffer + (width * height * 4);
    Common::unfold<u8, &color_a8>(read_cursor, write_cursor, width * height);
}

inline void color_ia8(u8*& read_cursor, u8*& write_cursor) {
    read_cursor -= 2;
    write_cursor -= 4;
    u32 tmp = 0;
    u16 tmp2;
    std::memcpy(&tmp2, read_cursor, sizeof(u16));
    tmp = tmp2 & 0x0000FF00;
    tmp2 = tmp2 & 0x00FF;
    tmp = (tmp << 8) | (tmp >> 8) | tmp | (tmp2 << 24);
    std::memcpy(write_cursor, &tmp, sizeof(u32));
}

void IA8(u8* target_buffer, u32 width, u32 height) {
    u8* read_cursor = target_buffer + (width * height * 2);
    u8* write_cursor = target_buffer + (width * height * 4);
    Common::unfold<u8, &color_ia8>(read_cursor, write_cursor, width * height);
}

} // Decoders

} // Pica
