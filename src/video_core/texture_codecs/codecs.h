
#pragma once

#include "common/common_types.h"

namespace Pica {

namespace Encoders {

/**
 * Encodes textures using matrix data into z-order/morton-order.
 * encodes matrix_buffer -> morton_buffer
 * @param matrix_buffer pointer to an image in matrix order
 * @param morton_buffer pointer to an image using z-order
 * @param width texture's width
 * @param height texture's height
 * @param bytespp bytes per pixel
 */
bool Morton(u8* matrix_buffer, u8* morton_buffer, u32 width, u32 height, u32 bytespp);

/**
 * Encodes textures using 24Depth 8 Stencil (D24S8) to 8Stencil 24Depth (S8D24)
 * @param buffer pointer to a buffer in D24S8.
 * @param width texture's width
 * @param height texture's height
 * @param bytespp bytes per pixel
 */
void Depth(u8* target_buffer, u32 width, u32 height);

} // Encoders

namespace Decoders {

/**
 * Decodes textures using z-order/morton-order into matrix data.
 * decodes morton_buffer -> matrix_buffer
 * @param morton_buffer pointer to an image using z-order
 * @param matrix_buffer pointer to an image in matrix order
 * @param width texture's width
 * @param height texture's height
 * @param bytespp bytes per pixel
 */
bool Morton(u8* morton_buffer, u8* matrix_buffer, u32 width, u32 height, u32 bytespp);

/**
 * swaps all the bytes in a 32 bits aligned buffer; decodes from big-endian to
 * little endian.
 * @param target_buffer pointer to a buffer in big endian
 * @param width texture's width
 * @param height texture's height
 * @param bytespp bytes per pixel
 */
void BigEndian(u8* target_buffer, u32 width, u32 height);

/**
 * Decodes textures using 8Stencil 24Depth (S8D24) to 24Depth 8 Stencil (D24S8)
 * @param buffer pointer to a buffer in S8D24.
 * @param width texture's width
 * @param height texture's height
 * @param bytespp bytes per pixel
 */
void Depth(u8* target_buffer, u32 width, u32 height);

void I8(u8* target_buffer, u32 width, u32 height);
void A8(u8* target_buffer, u32 width, u32 height);
void IA8(u8* target_buffer, u32 width, u32 height);

} // Decoders

} // Pica
