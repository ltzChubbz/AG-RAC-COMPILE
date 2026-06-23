#include "vif.h"
#include <string.h>
#include <stdio.h>

u32 vif_parse_sector(const u8 *data, u32 size, VifVertex *out_verts, u32 max_verts) {
    u32 ptr = 0;
    u32 v_count = 0;
    u32 uv_count = 0;
    
    /* 
     * In R&C1, a sector often contains multiple UNPACK packets.
     * We track the current vertex and UV indices to link them.
     */
    while (ptr < size - 4) {
        u8 cmd = data[ptr + 3];
        u8 num = data[ptr + 2];
        u32 imm = data[ptr] | (data[ptr+1] << 8);
        
        // --- STCYCL (0x01) ---
        if (cmd == 0x01) {
            ptr += 4;
            continue;
        }
        
        // --- UNPACK (0x60+) ---
        if ((cmd & 0x60) == 0x60) {
            u8 v_type = (cmd >> 2) & 0x03; // 0=S, 1=V2, 2=V3, 3=V4
            u8 d_type = cmd & 0x03;        // 0=32b, 1=16b, 2=8b, 3=5b
            u32 data_ptr = ptr + 4;
            
            // V4-16 (Position + Flag) - Standard for R&C1 World Geometry
            if (v_type == 3 && d_type == 1) {
                const s16 *raw = (const s16*)&data[data_ptr];
                for (u32 i = 0; i < num && v_count < max_verts; i++) {
                    if (data_ptr + i * 8 + 8 > size) break; // Bounds check
                    out_verts[v_count].x = raw[i*4 + 0];
                    out_verts[v_count].y = raw[i*4 + 1];
                    out_verts[v_count].z = raw[i*4 + 2];
                    out_verts[v_count].flag = (u16)raw[i*4 + 3];
                    v_count++;
                }
            }
            // V2-16 (ST / UVs)
            else if (v_type == 1 && d_type == 1) {
                const s16 *raw = (const s16*)&data[data_ptr];
                for (u32 i = 0; i < num && uv_count < max_verts; i++) {
                    if (data_ptr + i * 4 + 4 > size) break; // Bounds check
                    out_verts[uv_count].u = raw[i*2 + 0] / 4096.0f;
                    out_verts[uv_count].v = raw[i*2 + 1] / 4096.0f;
                    uv_count++;
                }
            }
            
            // Advance ptr: 4 (header) + bytes_per_element * num
            u32 row_size = (v_type + 1); // e.g. V4 = 4 elements
            u32 elem_size = 4;
            if (d_type == 1) elem_size = 2;
            else if (d_type == 2) elem_size = 1;
            else if (d_type == 3) { row_size = 1; elem_size = 2; } // V4-5 is 16-bits total per vector

            u32 payload_bytes = num * row_size * elem_size;
            ptr += 4 + ((payload_bytes + 3) & ~3);
        }
        else {
            ptr += 1; // Align search
        }
    }
    
    printf("[VIF] Parsed Sector: %u vertices, %u UVs\n", v_count, uv_count);
    for (u32 i = 0; i < 3 && i < v_count; i++) {
        printf("[VIF] Vert %u: X=%f Y=%f Z=%f\n", i, out_verts[i].x, out_verts[i].y, out_verts[i].z);
    }
    fflush(stdout);
    
    return v_count;
}
