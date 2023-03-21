#include <yaul.h>
#include <stdio.h>
#include <stdlib.h>
#include <mic3d.h>
#include "cloth-sim.h"

vdp1_gouraud_table_t _pool_shading_tables[CMDT_COUNT] __aligned(16);
vdp1_gouraud_table_t _pool_shading_tables2[512] __aligned(16);

static smpc_peripheral_digital_t _digital;

static void _vblank_out_handler(void *work __unused) {
    smpc_peripheral_intback_issue();
}

void
main(void)
{
        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        vdp1_vram_partitions_t vdp1_vram_partitions;

        vdp1_vram_partitions_get(&vdp1_vram_partitions);

        mic3d_init();

        light_gst_set(_pool_shading_tables,
            CMDT_COUNT,
            (vdp1_vram_t)(vdp1_vram_partitions.gouraud_base + 512));

        camera_t camera __unused;

        camera.position.x = FIX16(  0.0f);
        camera.position.y = FIX16(  0.0f);
        camera.position.z = FIX16(-30.0f);

        camera.target.x = FIX16_ZERO;
        camera.target.y = FIX16_ZERO;
        camera.target.z = FIX16_ZERO;

        camera.up.x =  FIX16_ZERO;
        camera.up.y = -FIX16_ONE;

        camera_lookat(&camera);

        // Initialize cloth simulator
        uint32_t quads = ClothSimInitialize();

        for (uint32_t i = 0; i < quads; i++) {
                const rgb1555_t color = RGB1555(1,
                                                fix16_int32_to(fix16_int32_from(i * 31) / (uint32_t)quads),
                                                fix16_int32_to(fix16_int32_from(i * 31) / (uint32_t)quads),
                                                fix16_int32_to(fix16_int32_from(i * 31) / (uint32_t)quads));

                _pool_shading_tables2[i].colors[0] = color;
                _pool_shading_tables2[i].colors[1] = color;
                _pool_shading_tables2[i].colors[2] = color;
                _pool_shading_tables2[i].colors[3] = color;
        }

        gst_set((vdp1_vram_t)vdp1_vram_partitions.gouraud_base);
        gst_put(_pool_shading_tables2, quads);
        gst_unset();

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                dbgio_puts("[H[2J");

                ClothMove(&_digital);

                // Frame tick cloth simulation
                ClothSimTick();

                render();

                vdp1_sync_render();

                vdp1_sync();
                vdp1_sync_wait();

                dbgio_flush();
                vdp2_sync();
                vdp2_sync_wait();
        }
}

void
user_init(void)
{
        smpc_peripheral_init();
        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_B,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));

        vdp1_sync_interval_set(-1);

        vdp1_env_default_set();

        vdp2_sprite_priority_set(0, 6);

        vdp2_tvmd_display_set();

        vdp2_sync();
        vdp2_sync_wait();
}
