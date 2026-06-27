#include "decompress.h"
#include <string.h>
#include <stdio.h>

u8* decompress_wad_block(u8* src, u8* src_end, u8* dest, u32 dest_size, u32* out_size) {
    if (memcmp(src, "WAD", 3) != 0) return src + 1;
    
    u32 compressed_size = *(u32*)(src + 3);
    u8* ptr = src + 0x10;
    u8* end = src + 0x10 + compressed_size;
    if (end > src_end || end < src) {
        end = src_end;
    }
    u8* begin = ptr;
    
    u32 written = 0;
    u32 current_dest_size = dest_size;
    
    while (ptr < end) {
        // Check for pad packet
        if (ptr + 3 <= end && ptr[0] == 0x12 && ptr[1] == 0x00 && ptr[2] == 0x00) {
            ptr += 3;
            while (ptr < end && *ptr == 0xee) ptr++;
            continue;
        }
        
        u8 flag = *ptr++;
        u8* curr_ptr = ptr - 1;
        u32 m = 0;
        s32 lb = -1;
        
        if (flag < 0x10) {
            u32 lit_size = (flag != 0) ? (flag + 3) : (*ptr++ + 18);
            if (dest_size == 55987) {
                printf("[C-TR] lit flag=0x%02X size=%u pos=%u dest_pos=%u\n", flag, lit_size, (u32)(curr_ptr - src), current_dest_size);
            }
            memcpy(dest + current_dest_size, ptr, lit_size);
            ptr += lit_size;
            current_dest_size += lit_size;
            written += lit_size;
        } else {
            if (flag < 0x20) {
                m = flag & 7;
                if (m == 0) m = *ptr++ + 7;
                u8 b0 = *ptr++; u8 b1 = *ptr++;
                lb = current_dest_size - ((flag & 8) * 0x800) - (b1 * 0x40) - (b0 >> 2);
                if ((u32)lb != current_dest_size) {
                    m += 2;
                    lb -= 0x4000;
                } else if (m != 1) {
                    if (dest_size == 55987) {
                        printf("[C-TR] align flag=0x%02X pos=%u\n", flag, (u32)(curr_ptr - src));
                    }
                    // Padding within block
                    while (ptr < end && ((u32)(ptr - begin) % 0x1000 != 0)) ptr++;
                    continue;
                }
            } else if (flag < 0x40) {
                m = (flag & 0x1f);
                if (m == 0) m = *ptr++ + 0x1f;
                m += 2;
                u8 b1 = *ptr++; u8 b2 = *ptr++;
                lb = current_dest_size - (b2 * 0x40) - (b1 >> 2) - 1;
            } else {
                u8 b1 = *ptr++;
                lb = current_dest_size - (b1 * 8) - ((flag >> 2) & 7) - 1;
                m = (flag >> 5) + 1;
            }
            
            if (dest_size == 55987) {
                printf("[C-TR] match flag=0x%02X m=%u lb=%d pos=%u dest_pos=%u\n", flag, m, lb, (u32)(curr_ptr - src), current_dest_size);
            }
            
            if (m != 1) {
                if (lb < 0 || (u32)lb >= current_dest_size) {
                    memset(dest + current_dest_size, 0, m);
                } else {
                    for (u32 i = 0; i < m; i++) {
                        dest[current_dest_size + i] = dest[lb + i];
                    }
                }
                current_dest_size += m;
                written += m;
            }
            
            // Little literal
            u8 lit = ptr[-2] & 3;
            if (lit > 0) {
                if (dest_size == 55987) {
                    printf("[C-TR] lit2 size=%u dest_pos=%u\n", lit, current_dest_size);
                }
                memcpy(dest + current_dest_size, ptr, lit);
                ptr += lit;
                current_dest_size += lit;
                written += lit;
            }
        }
    }
    
    if (out_size) *out_size = written;
    return end;
}
