        /* NMOS control-store block. */

        /* NMOS service slots. */

        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_service_mode, 0)] = &interrupt_resolver,

        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_illegal_ext, 0)] = &mc_dispatch,

        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_internal_reset, 0)] = &mc_internal_reset,

        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_reset_0, 0)] = &mc_restart_read_hhff,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_reset_0, 1)] = &mc_restart_read_zzhh_fetch,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_reset_0, 2)] = &mc_restart_read_zzhh,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_reset_0, 3)] = &mc_reset_stack_read,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_reset_0, 4)] = &mc_reset_stack_read,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_reset_0, 5)] = &mc_reset_stack_read,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_reset_0, 6)] = &mc_reset_vec_lo,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_reset_0, 7)] = &mc_reset_vec_hi,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_reset_1, 0)] = &mc_latch_pch_fetch,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_reset_1, 1)] = &mc_dispatch,

        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_goto, 0)] = &mc_fetch,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_goto, 1)] = &mc_dispatch,

        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_nmi, 0)] = &mc_read_pc,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_nmi, 1)] = &mc_stack_push_pc_high,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_nmi, 2)] = &mc_stack_push_pc_low,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_nmi, 3)] = &mc_nmi_c3_push_p,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_nmi, 4)] = &mc_nmi_c4_vec_lo,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_nmi, 5)] = &mc_nmos_nmi_c5_vec_hi,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_nmi, 6)] = &mc_latch_pch_fetch,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_nmi, 7)] = &mc_dispatch,

        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_irq, 0)] = &mc_read_pc,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_irq, 1)] = &mc_stack_push_pc_high,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_irq, 2)] = &mc_stack_push_pc_low,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_irq, 3)] = &mc_irq_c3_push_p,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_irq, 4)] = &mc_irq_c4_vec_lo,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_irq, 5)] = &mc_nmos_irq_c5_vec_hi,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_irq, 6)] = &mc_latch_pch_fetch,
        [SERVICE_SLOT_IDX(qe6502_model_nmos, service_slot_irq, 7)] = &mc_dispatch,

        /* NMOS opcode slots. */

        /* 0x00 BRK ; class=brk */
        [IDX(qe6502_model_nmos, 0x00, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x00, 1)] = &mc_stack_push_pc_high,
        [IDX(qe6502_model_nmos, 0x00, 2)] = &mc_stack_push_pc_low,
        [IDX(qe6502_model_nmos, 0x00, 3)] = &mc_stack_push_status_b,
        [IDX(qe6502_model_nmos, 0x00, 4)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x00, 5)] = &mc_brk_c5_vec_hi,
        [IDX(qe6502_model_nmos, 0x00, 6)] = &mc_latch_pch_fetch,
        [IDX(qe6502_model_nmos, 0x00, 7)] = &mc_dispatch,

        /* 0x01 ORA (zp,X) ; class=r_izx */
        [IDX(qe6502_model_nmos, 0x01, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x01, 1)] = &mc_r_izx_c1_idx,
        [IDX(qe6502_model_nmos, 0x01, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0x01, 3)] = &mc_latch_data_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x01, 4)] = &mc_r_izx_c4_data_no_latch_prefetch,
        [IDX(qe6502_model_nmos, 0x01, 5)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x01, 6)] = &mc_dispatch,

        /* 0x02 KIL/JAM ; class=kil_jam */
        [IDX(qe6502_model_nmos, 0x02, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x02, 1)] = &mc_kil_jam_read_ffff,
        [IDX(qe6502_model_nmos, 0x02, 2)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x02, 3)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x02, 4)] = &op_kil_jam_r_ffff_pending_data_loop,

        /* 0x03 SLO (zp,X) ; class=rw_izx */
        [IDX(qe6502_model_nmos, 0x03, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x03, 1)] = &mc_r_izx_c1_idx,
        [IDX(qe6502_model_nmos, 0x03, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0x03, 3)] = &mc_latch_data_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x03, 4)] = &mc_r_izx_c4_data,
        [IDX(qe6502_model_nmos, 0x03, 5)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x03, 6)] = &op_slo_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x03, 7)] = &mc_fetch_illegal_ext_dispatch,

        /* 0x04 NOP zp ; class=nop_zp_read */
        [IDX(qe6502_model_nmos, 0x04, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x04, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0x04, 2)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x04, 3)] = &mc_dispatch,

        /* 0x05 ORA zp ; class=r_zp */
        [IDX(qe6502_model_nmos, 0x05, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x05, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0x05, 2)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x05, 3)] = &mc_dispatch,

        /* 0x06 ASL zp ; class=rw_zp */
        [IDX(qe6502_model_nmos, 0x06, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x06, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x06, 2)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x06, 3)] = &op_asl_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x06, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x06, 5)] = &mc_dispatch,

        /* 0x07 SLO zp ; class=rw_zp */
        [IDX(qe6502_model_nmos, 0x07, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x07, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x07, 2)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x07, 3)] = &op_slo_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x07, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x07, 5)] = &mc_dispatch,

        /* 0x08 PHP ; class=stack_push */
        [IDX(qe6502_model_nmos, 0x08, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x08, 1)] = &op_php_stack_push_ready_none_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x08, 2)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x08, 3)] = &mc_dispatch,

        /* 0x09 ORA #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0x09, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0x09, 1)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x09, 2)] = &mc_dispatch,

        /* 0x0A ASL A ; class=acc */
        [IDX(qe6502_model_nmos, 0x0A, 0)] = &op_asl_acc_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0x0A, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x0A, 2)] = &mc_dispatch,

        /* 0x0B ANC #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0x0B, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0x0B, 1)] = &op_anc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x0B, 2)] = &mc_dispatch,


        /* 0x0C NOP abs ; class=nop_abs_read */
        [IDX(qe6502_model_nmos, 0x0C, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x0C, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x0C, 2)] = &mc_read_addr_with_bus_hi_prefetch,
        [IDX(qe6502_model_nmos, 0x0C, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x0C, 4)] = &mc_dispatch,

        /* 0x0D ORA abs ; class=r_abs */
        [IDX(qe6502_model_nmos, 0x0D, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x0D, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x0D, 2)] = &mc_read_addr_with_bus_hi_prefetch,
        [IDX(qe6502_model_nmos, 0x0D, 3)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x0D, 4)] = &mc_dispatch,

        /* 0x0E ASL abs ; class=rw_abs */
        [IDX(qe6502_model_nmos, 0x0E, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x0E, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x0E, 2)] = &mc_latch_addr1_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x0E, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x0E, 4)] = &op_asl_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x0E, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x0E, 6)] = &mc_dispatch,

        /* 0x0F SLO abs ; class=rw_abs */
        [IDX(qe6502_model_nmos, 0x0F, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x0F, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x0F, 2)] = &mc_latch_addr1_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x0F, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x0F, 4)] = &op_slo_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x0F, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x0F, 6)] = &mc_dispatch,

        /* 0x10 BPL rel ; class=branch_rel */
        [IDX(qe6502_model_nmos, 0x10, 0)] = &op_bpl_branch_c0_offset_prefetch,
        [IDX(qe6502_model_nmos, 0x10, 1)] = &mc_branch_rel_c1_apply,
        [IDX(qe6502_model_nmos, 0x10, 2)] = &mc_branch_pgfix_prefetch,
        [IDX(qe6502_model_nmos, 0x10, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x10, 4)] = &mc_dispatch,

        /* 0x11 ORA (zp),Y ; class=r_izy */
        [IDX(qe6502_model_nmos, 0x11, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x11, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x11, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0x11, 3)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x11, 4)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x11, 5)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x11, 6)] = &mc_dispatch,

        /* 0x12 KIL/JAM ; class=kil_jam */
        [IDX(qe6502_model_nmos, 0x12, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x12, 1)] = &mc_kil_jam_read_ffff,
        [IDX(qe6502_model_nmos, 0x12, 2)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x12, 3)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x12, 4)] = &op_kil_jam_r_ffff_pending_data_loop,

        /* 0x13 SLO (zp),Y ; class=rw_izy */
        [IDX(qe6502_model_nmos, 0x13, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x13, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x13, 2)] = &mc_w_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0x13, 3)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x13, 4)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x13, 5)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x13, 6)] = &op_slo_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x13, 7)] = &mc_fetch_illegal_ext_dispatch,

        /* 0x14 NOP zp,X ; class=nop_zpx_read */
        [IDX(qe6502_model_nmos, 0x14, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x14, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x14, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x14, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x14, 4)] = &mc_dispatch,

        /* 0x15 ORA zp,X ; class=r_zpx */
        [IDX(qe6502_model_nmos, 0x15, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x15, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x15, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x15, 3)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x15, 4)] = &mc_dispatch,

        /* 0x16 ASL zp,X ; class=rw_zpx */
        [IDX(qe6502_model_nmos, 0x16, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x16, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x16, 2)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x16, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x16, 4)] = &op_asl_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x16, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x16, 6)] = &mc_dispatch,

        /* 0x17 SLO zp,X ; class=rw_zpx */
        [IDX(qe6502_model_nmos, 0x17, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x17, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x17, 2)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x17, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x17, 4)] = &op_slo_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x17, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x17, 6)] = &mc_dispatch,

        /* 0x18 CLC ; class=imp */
        [IDX(qe6502_model_nmos, 0x18, 0)] = &op_clc_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0x18, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x18, 2)] = &mc_dispatch,

        /* 0x19 ORA abs,Y ; class=r_aby */
        [IDX(qe6502_model_nmos, 0x19, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x19, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x19, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x19, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x19, 4)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x19, 5)] = &mc_dispatch,

        /* 0x1A NOP ; class=nop_imp */
        [IDX(qe6502_model_nmos, 0x1A, 0)] = &mc_read_pc_prefetch,
        [IDX(qe6502_model_nmos, 0x1A, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x1A, 2)] = &mc_dispatch,

        /* 0x1B SLO abs,Y ; class=rw_aby */
        [IDX(qe6502_model_nmos, 0x1B, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x1B, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x1B, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x1B, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x1B, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x1B, 5)] = &op_slo_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x1B, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x1B, 7)] = &mc_dispatch,

        /* 0x1C NOP abs,X ; class=nop_abx_read */
        [IDX(qe6502_model_nmos, 0x1C, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x1C, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x1C, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x1C, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x1C, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x1C, 5)] = &mc_dispatch,

        /* 0x1D ORA abs,X ; class=r_abx */
        [IDX(qe6502_model_nmos, 0x1D, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x1D, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x1D, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x1D, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x1D, 4)] = &op_ora_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x1D, 5)] = &mc_dispatch,

        /* 0x1E ASL abs,X ; class=rw_abx */
        [IDX(qe6502_model_nmos, 0x1E, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x1E, 1)] = &mc_rw_abx_c1_hi,
        [IDX(qe6502_model_nmos, 0x1E, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x1E, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x1E, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x1E, 5)] = &op_asl_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x1E, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x1E, 7)] = &mc_dispatch,

        /* 0x1F SLO abs,X ; class=rw_abx */
        [IDX(qe6502_model_nmos, 0x1F, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x1F, 1)] = &mc_rw_abx_c1_hi,
        [IDX(qe6502_model_nmos, 0x1F, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x1F, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x1F, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x1F, 5)] = &op_slo_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x1F, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x1F, 7)] = &mc_dispatch,

        /* 0x20 JSR abs ; class=jsr_abs */
        [IDX(qe6502_model_nmos, 0x20, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x20, 1)] = &mc_jsr_abs_c1_dummy,
        [IDX(qe6502_model_nmos, 0x20, 2)] = &mc_stack_push_pc_high,
        [IDX(qe6502_model_nmos, 0x20, 3)] = &mc_stack_push_pc_low,
        [IDX(qe6502_model_nmos, 0x20, 4)] = &mc_read_pc_prefetch,
        [IDX(qe6502_model_nmos, 0x20, 5)] = &mc_latch_addr1_set_pc_fetch,
        [IDX(qe6502_model_nmos, 0x20, 6)] = &mc_dispatch,

        /* 0x21 AND (zp,X) ; class=r_izx */
        [IDX(qe6502_model_nmos, 0x21, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x21, 1)] = &mc_r_izx_c1_idx,
        [IDX(qe6502_model_nmos, 0x21, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0x21, 3)] = &mc_latch_data_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x21, 4)] = &mc_r_izx_c4_data_no_latch_prefetch,
        [IDX(qe6502_model_nmos, 0x21, 5)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x21, 6)] = &mc_dispatch,

        /* 0x22 KIL/JAM ; class=kil_jam */
        [IDX(qe6502_model_nmos, 0x22, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x22, 1)] = &mc_kil_jam_read_ffff,
        [IDX(qe6502_model_nmos, 0x22, 2)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x22, 3)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x22, 4)] = &op_kil_jam_r_ffff_pending_data_loop,

        /* 0x23 RLA (zp,X) ; class=rw_izx */
        [IDX(qe6502_model_nmos, 0x23, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x23, 1)] = &mc_r_izx_c1_idx,
        [IDX(qe6502_model_nmos, 0x23, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0x23, 3)] = &mc_latch_data_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x23, 4)] = &mc_r_izx_c4_data,
        [IDX(qe6502_model_nmos, 0x23, 5)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x23, 6)] = &op_rla_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x23, 7)] = &mc_fetch_illegal_ext_dispatch,

        /* 0x24 BIT zp ; class=r_zp */
        [IDX(qe6502_model_nmos, 0x24, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x24, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0x24, 2)] = &op_bit_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x24, 3)] = &mc_dispatch,

        /* 0x25 AND zp ; class=r_zp */
        [IDX(qe6502_model_nmos, 0x25, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x25, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0x25, 2)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x25, 3)] = &mc_dispatch,

        /* 0x26 ROL zp ; class=rw_zp */
        [IDX(qe6502_model_nmos, 0x26, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x26, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x26, 2)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x26, 3)] = &op_rol_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x26, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x26, 5)] = &mc_dispatch,

        /* 0x27 RLA zp ; class=rw_zp */
        [IDX(qe6502_model_nmos, 0x27, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x27, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x27, 2)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x27, 3)] = &op_rla_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x27, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x27, 5)] = &mc_dispatch,

        /* 0x28 PLP ; class=stack_pull */
        [IDX(qe6502_model_nmos, 0x28, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x28, 1)] = &mc_read_stack_current_s,
        [IDX(qe6502_model_nmos, 0x28, 2)] = &mc_stack_pull_read_prefetch,
        [IDX(qe6502_model_nmos, 0x28, 3)] = &op_plp_stack_pull_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x28, 4)] = &mc_dispatch,

        /* 0x29 AND #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0x29, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0x29, 1)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x29, 2)] = &mc_dispatch,

        /* 0x2A ROL A ; class=acc */
        [IDX(qe6502_model_nmos, 0x2A, 0)] = &op_rol_acc_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0x2A, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x2A, 2)] = &mc_dispatch,

        /* 0x2B ANC #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0x2B, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0x2B, 1)] = &op_anc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x2B, 2)] = &mc_dispatch,


        /* 0x2C BIT abs ; class=r_abs */
        [IDX(qe6502_model_nmos, 0x2C, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x2C, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x2C, 2)] = &mc_read_addr_with_bus_hi_prefetch,
        [IDX(qe6502_model_nmos, 0x2C, 3)] = &op_bit_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x2C, 4)] = &mc_dispatch,

        /* 0x2D AND abs ; class=r_abs */
        [IDX(qe6502_model_nmos, 0x2D, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x2D, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x2D, 2)] = &mc_read_addr_with_bus_hi_prefetch,
        [IDX(qe6502_model_nmos, 0x2D, 3)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x2D, 4)] = &mc_dispatch,

        /* 0x2E ROL abs ; class=rw_abs */
        [IDX(qe6502_model_nmos, 0x2E, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x2E, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x2E, 2)] = &mc_latch_addr1_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x2E, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x2E, 4)] = &op_rol_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x2E, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x2E, 6)] = &mc_dispatch,

        /* 0x2F RLA abs ; class=rw_abs */
        [IDX(qe6502_model_nmos, 0x2F, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x2F, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x2F, 2)] = &mc_latch_addr1_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x2F, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x2F, 4)] = &op_rla_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x2F, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x2F, 6)] = &mc_dispatch,

        /* 0x30 BMI rel ; class=branch_rel */
        [IDX(qe6502_model_nmos, 0x30, 0)] = &op_bmi_branch_c0_offset_prefetch,
        [IDX(qe6502_model_nmos, 0x30, 1)] = &mc_branch_rel_c1_apply,
        [IDX(qe6502_model_nmos, 0x30, 2)] = &mc_branch_pgfix_prefetch,
        [IDX(qe6502_model_nmos, 0x30, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x30, 4)] = &mc_dispatch,

        /* 0x31 AND (zp),Y ; class=r_izy */
        [IDX(qe6502_model_nmos, 0x31, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x31, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x31, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0x31, 3)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x31, 4)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x31, 5)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x31, 6)] = &mc_dispatch,

        /* 0x32 KIL/JAM ; class=kil_jam */
        [IDX(qe6502_model_nmos, 0x32, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x32, 1)] = &mc_kil_jam_read_ffff,
        [IDX(qe6502_model_nmos, 0x32, 2)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x32, 3)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x32, 4)] = &op_kil_jam_r_ffff_pending_data_loop,

        /* 0x33 RLA (zp),Y ; class=rw_izy */
        [IDX(qe6502_model_nmos, 0x33, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x33, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x33, 2)] = &mc_w_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0x33, 3)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x33, 4)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x33, 5)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x33, 6)] = &op_rla_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x33, 7)] = &mc_fetch_illegal_ext_dispatch,

        /* 0x34 NOP zp,X ; class=nop_zpx_read */
        [IDX(qe6502_model_nmos, 0x34, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x34, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x34, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x34, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x34, 4)] = &mc_dispatch,

        /* 0x35 AND zp,X ; class=r_zpx */
        [IDX(qe6502_model_nmos, 0x35, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x35, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x35, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x35, 3)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x35, 4)] = &mc_dispatch,

        /* 0x36 ROL zp,X ; class=rw_zpx */
        [IDX(qe6502_model_nmos, 0x36, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x36, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x36, 2)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x36, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x36, 4)] = &op_rol_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x36, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x36, 6)] = &mc_dispatch,

        /* 0x37 RLA zp,X ; class=rw_zpx */
        [IDX(qe6502_model_nmos, 0x37, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x37, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x37, 2)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x37, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x37, 4)] = &op_rla_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x37, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x37, 6)] = &mc_dispatch,

        /* 0x38 SEC ; class=imp */
        [IDX(qe6502_model_nmos, 0x38, 0)] = &op_sec_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0x38, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x38, 2)] = &mc_dispatch,

        /* 0x39 AND abs,Y ; class=r_aby */
        [IDX(qe6502_model_nmos, 0x39, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x39, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x39, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x39, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x39, 4)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x39, 5)] = &mc_dispatch,

        /* 0x3A NOP ; class=nop_imp */
        [IDX(qe6502_model_nmos, 0x3A, 0)] = &mc_read_pc_prefetch,
        [IDX(qe6502_model_nmos, 0x3A, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x3A, 2)] = &mc_dispatch,

        /* 0x3B RLA abs,Y ; class=rw_aby */
        [IDX(qe6502_model_nmos, 0x3B, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x3B, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x3B, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x3B, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x3B, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x3B, 5)] = &op_rla_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x3B, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x3B, 7)] = &mc_dispatch,

        /* 0x3C NOP abs,X ; class=nop_abx_read */
        [IDX(qe6502_model_nmos, 0x3C, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x3C, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x3C, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x3C, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x3C, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x3C, 5)] = &mc_dispatch,

        /* 0x3D AND abs,X ; class=r_abx */
        [IDX(qe6502_model_nmos, 0x3D, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x3D, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x3D, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x3D, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x3D, 4)] = &op_and_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x3D, 5)] = &mc_dispatch,

        /* 0x3E ROL abs,X ; class=rw_abx */
        [IDX(qe6502_model_nmos, 0x3E, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x3E, 1)] = &mc_rw_abx_c1_hi,
        [IDX(qe6502_model_nmos, 0x3E, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x3E, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x3E, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x3E, 5)] = &op_rol_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x3E, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x3E, 7)] = &mc_dispatch,

        /* 0x3F RLA abs,X ; class=rw_abx */
        [IDX(qe6502_model_nmos, 0x3F, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x3F, 1)] = &mc_rw_abx_c1_hi,
        [IDX(qe6502_model_nmos, 0x3F, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x3F, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x3F, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x3F, 5)] = &op_rla_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x3F, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x3F, 7)] = &mc_dispatch,

        /* 0x40 RTI ; class=rti */
        [IDX(qe6502_model_nmos, 0x40, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x40, 1)] = &mc_read_stack_current_s,
        [IDX(qe6502_model_nmos, 0x40, 2)] = &mc_stack_pull_read,
        [IDX(qe6502_model_nmos, 0x40, 3)] = &mc_rti_c3_pull_pcl,
        [IDX(qe6502_model_nmos, 0x40, 4)] = &mc_latch_pcl_stack_read_prefetch,
        [IDX(qe6502_model_nmos, 0x40, 5)] = &mc_latch_pch_fetch,
        [IDX(qe6502_model_nmos, 0x40, 6)] = &mc_dispatch,

        /* 0x41 EOR (zp,X) ; class=r_izx */
        [IDX(qe6502_model_nmos, 0x41, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x41, 1)] = &mc_r_izx_c1_idx,
        [IDX(qe6502_model_nmos, 0x41, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0x41, 3)] = &mc_latch_data_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x41, 4)] = &mc_r_izx_c4_data_no_latch_prefetch,
        [IDX(qe6502_model_nmos, 0x41, 5)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x41, 6)] = &mc_dispatch,

        /* 0x42 KIL/JAM ; class=kil_jam */
        [IDX(qe6502_model_nmos, 0x42, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x42, 1)] = &mc_kil_jam_read_ffff,
        [IDX(qe6502_model_nmos, 0x42, 2)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x42, 3)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x42, 4)] = &op_kil_jam_r_ffff_pending_data_loop,

        /* 0x43 SRE (zp,X) ; class=rw_izx */
        [IDX(qe6502_model_nmos, 0x43, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x43, 1)] = &mc_r_izx_c1_idx,
        [IDX(qe6502_model_nmos, 0x43, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0x43, 3)] = &mc_latch_data_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x43, 4)] = &mc_r_izx_c4_data,
        [IDX(qe6502_model_nmos, 0x43, 5)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x43, 6)] = &op_sre_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x43, 7)] = &mc_fetch_illegal_ext_dispatch,

        /* 0x44 NOP zp ; class=nop_zp_read */
        [IDX(qe6502_model_nmos, 0x44, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x44, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0x44, 2)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x44, 3)] = &mc_dispatch,

        /* 0x45 EOR zp ; class=r_zp */
        [IDX(qe6502_model_nmos, 0x45, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x45, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0x45, 2)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x45, 3)] = &mc_dispatch,

        /* 0x46 LSR zp ; class=rw_zp */
        [IDX(qe6502_model_nmos, 0x46, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x46, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x46, 2)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x46, 3)] = &op_lsr_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x46, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x46, 5)] = &mc_dispatch,

        /* 0x47 SRE zp ; class=rw_zp */
        [IDX(qe6502_model_nmos, 0x47, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x47, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x47, 2)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x47, 3)] = &op_sre_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x47, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x47, 5)] = &mc_dispatch,

        /* 0x48 PHA ; class=stack_push */
        [IDX(qe6502_model_nmos, 0x48, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x48, 1)] = &op_pha_stack_push_ready_none_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x48, 2)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x48, 3)] = &mc_dispatch,

        /* 0x49 EOR #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0x49, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0x49, 1)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x49, 2)] = &mc_dispatch,

        /* 0x4A LSR A ; class=acc */
        [IDX(qe6502_model_nmos, 0x4A, 0)] = &op_lsr_acc_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0x4A, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x4A, 2)] = &mc_dispatch,

        /* 0x4B ALR #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0x4B, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0x4B, 1)] = &op_alr_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x4B, 2)] = &mc_dispatch,


        /* 0x4C JMP abs ; class=jmp_abs */
        [IDX(qe6502_model_nmos, 0x4C, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x4C, 1)] = &mc_latch_addr0_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0x4C, 2)] = &mc_latch_addr1_set_pc_fetch,
        [IDX(qe6502_model_nmos, 0x4C, 3)] = &mc_dispatch,

        /* 0x4D EOR abs ; class=r_abs */
        [IDX(qe6502_model_nmos, 0x4D, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x4D, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x4D, 2)] = &mc_read_addr_with_bus_hi_prefetch,
        [IDX(qe6502_model_nmos, 0x4D, 3)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x4D, 4)] = &mc_dispatch,

        /* 0x4E LSR abs ; class=rw_abs */
        [IDX(qe6502_model_nmos, 0x4E, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x4E, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x4E, 2)] = &mc_latch_addr1_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x4E, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x4E, 4)] = &op_lsr_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x4E, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x4E, 6)] = &mc_dispatch,

        /* 0x4F SRE abs ; class=rw_abs */
        [IDX(qe6502_model_nmos, 0x4F, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x4F, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x4F, 2)] = &mc_latch_addr1_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x4F, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x4F, 4)] = &op_sre_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x4F, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x4F, 6)] = &mc_dispatch,

        /* 0x50 BVC rel ; class=branch_rel */
        [IDX(qe6502_model_nmos, 0x50, 0)] = &op_bvc_branch_c0_offset_prefetch,
        [IDX(qe6502_model_nmos, 0x50, 1)] = &mc_branch_rel_c1_apply,
        [IDX(qe6502_model_nmos, 0x50, 2)] = &mc_branch_pgfix_prefetch,
        [IDX(qe6502_model_nmos, 0x50, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x50, 4)] = &mc_dispatch,

        /* 0x51 EOR (zp),Y ; class=r_izy */
        [IDX(qe6502_model_nmos, 0x51, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x51, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x51, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0x51, 3)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x51, 4)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x51, 5)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x51, 6)] = &mc_dispatch,

        /* 0x52 KIL/JAM ; class=kil_jam */
        [IDX(qe6502_model_nmos, 0x52, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x52, 1)] = &mc_kil_jam_read_ffff,
        [IDX(qe6502_model_nmos, 0x52, 2)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x52, 3)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x52, 4)] = &op_kil_jam_r_ffff_pending_data_loop,

        /* 0x53 SRE (zp),Y ; class=rw_izy */
        [IDX(qe6502_model_nmos, 0x53, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x53, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x53, 2)] = &mc_w_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0x53, 3)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x53, 4)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x53, 5)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x53, 6)] = &op_sre_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x53, 7)] = &mc_fetch_illegal_ext_dispatch,

        /* 0x54 NOP zp,X ; class=nop_zpx_read */
        [IDX(qe6502_model_nmos, 0x54, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x54, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x54, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x54, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x54, 4)] = &mc_dispatch,

        /* 0x55 EOR zp,X ; class=r_zpx */
        [IDX(qe6502_model_nmos, 0x55, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x55, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x55, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x55, 3)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x55, 4)] = &mc_dispatch,

        /* 0x56 LSR zp,X ; class=rw_zpx */
        [IDX(qe6502_model_nmos, 0x56, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x56, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x56, 2)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x56, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x56, 4)] = &op_lsr_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x56, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x56, 6)] = &mc_dispatch,

        /* 0x57 SRE zp,X ; class=rw_zpx */
        [IDX(qe6502_model_nmos, 0x57, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x57, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x57, 2)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x57, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x57, 4)] = &op_sre_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x57, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x57, 6)] = &mc_dispatch,

        /* 0x58 CLI ; class=imp */
        [IDX(qe6502_model_nmos, 0x58, 0)] = &op_cli_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0x58, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x58, 2)] = &mc_dispatch,

        /* 0x59 EOR abs,Y ; class=r_aby */
        [IDX(qe6502_model_nmos, 0x59, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x59, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x59, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x59, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x59, 4)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x59, 5)] = &mc_dispatch,

        /* 0x5A NOP ; class=nop_imp */
        [IDX(qe6502_model_nmos, 0x5A, 0)] = &mc_read_pc_prefetch,
        [IDX(qe6502_model_nmos, 0x5A, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x5A, 2)] = &mc_dispatch,

        /* 0x5B SRE abs,Y ; class=rw_aby */
        [IDX(qe6502_model_nmos, 0x5B, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x5B, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x5B, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x5B, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x5B, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x5B, 5)] = &op_sre_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x5B, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x5B, 7)] = &mc_dispatch,

        /* 0x5C NOP abs,X ; class=nop_abx_read */
        [IDX(qe6502_model_nmos, 0x5C, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x5C, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x5C, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x5C, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x5C, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x5C, 5)] = &mc_dispatch,

        /* 0x5D EOR abs,X ; class=r_abx */
        [IDX(qe6502_model_nmos, 0x5D, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x5D, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x5D, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x5D, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x5D, 4)] = &op_eor_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x5D, 5)] = &mc_dispatch,

        /* 0x5E LSR abs,X ; class=rw_abx */
        [IDX(qe6502_model_nmos, 0x5E, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x5E, 1)] = &mc_rw_abx_c1_hi,
        [IDX(qe6502_model_nmos, 0x5E, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x5E, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x5E, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x5E, 5)] = &op_lsr_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x5E, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x5E, 7)] = &mc_dispatch,

        /* 0x5F SRE abs,X ; class=rw_abx */
        [IDX(qe6502_model_nmos, 0x5F, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x5F, 1)] = &mc_rw_abx_c1_hi,
        [IDX(qe6502_model_nmos, 0x5F, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x5F, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x5F, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x5F, 5)] = &op_sre_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x5F, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x5F, 7)] = &mc_dispatch,

        /* 0x60 RTS ; class=rts */
        [IDX(qe6502_model_nmos, 0x60, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x60, 1)] = &mc_read_stack_current_s,
        [IDX(qe6502_model_nmos, 0x60, 2)] = &mc_stack_pull_read,
        [IDX(qe6502_model_nmos, 0x60, 3)] = &mc_latch_pcl_stack_read,
        [IDX(qe6502_model_nmos, 0x60, 4)] = &mc_rts_c4_inc_pc_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0x60, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x60, 6)] = &mc_dispatch,

        /* 0x61 ADC (zp,X) ; class=r_izx */
        [IDX(qe6502_model_nmos, 0x61, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x61, 1)] = &mc_r_izx_c1_idx,
        [IDX(qe6502_model_nmos, 0x61, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0x61, 3)] = &mc_latch_data_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x61, 4)] = &mc_r_izx_c4_data_no_latch_prefetch,
        [IDX(qe6502_model_nmos, 0x61, 5)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x61, 6)] = &mc_dispatch,

        /* 0x62 KIL/JAM ; class=kil_jam */
        [IDX(qe6502_model_nmos, 0x62, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x62, 1)] = &mc_kil_jam_read_ffff,
        [IDX(qe6502_model_nmos, 0x62, 2)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x62, 3)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x62, 4)] = &op_kil_jam_r_ffff_pending_data_loop,

        /* 0x63 RRA (zp,X) ; class=rw_izx */
        [IDX(qe6502_model_nmos, 0x63, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x63, 1)] = &mc_r_izx_c1_idx,
        [IDX(qe6502_model_nmos, 0x63, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0x63, 3)] = &mc_latch_data_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x63, 4)] = &mc_r_izx_c4_data,
        [IDX(qe6502_model_nmos, 0x63, 5)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x63, 6)] = &op_rra_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x63, 7)] = &mc_fetch_illegal_ext_dispatch,

        /* 0x64 NOP zp ; class=nop_zp_read */
        [IDX(qe6502_model_nmos, 0x64, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x64, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0x64, 2)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x64, 3)] = &mc_dispatch,

        /* 0x65 ADC zp ; class=r_zp */
        [IDX(qe6502_model_nmos, 0x65, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x65, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0x65, 2)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x65, 3)] = &mc_dispatch,

        /* 0x66 ROR zp ; class=rw_zp */
        [IDX(qe6502_model_nmos, 0x66, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x66, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x66, 2)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x66, 3)] = &op_ror_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x66, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x66, 5)] = &mc_dispatch,

        /* 0x67 RRA zp ; class=rw_zp */
        [IDX(qe6502_model_nmos, 0x67, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x67, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x67, 2)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x67, 3)] = &op_rra_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x67, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x67, 5)] = &mc_dispatch,

        /* 0x68 PLA ; class=stack_pull */
        [IDX(qe6502_model_nmos, 0x68, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x68, 1)] = &mc_read_stack_current_s,
        [IDX(qe6502_model_nmos, 0x68, 2)] = &mc_stack_pull_read_prefetch,
        [IDX(qe6502_model_nmos, 0x68, 3)] = &mc_load_a_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0x68, 4)] = &mc_dispatch,

        /* 0x69 ADC #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0x69, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0x69, 1)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x69, 2)] = &mc_dispatch,

        /* 0x6A ROR A ; class=acc */
        [IDX(qe6502_model_nmos, 0x6A, 0)] = &op_ror_acc_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0x6A, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x6A, 2)] = &mc_dispatch,

        /* 0x6B ARR #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0x6B, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0x6B, 1)] = &op_arr_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x6B, 2)] = &mc_dispatch,

        /* 0x6C JMP (abs) ; class=jmp_ind */
        [IDX(qe6502_model_nmos, 0x6C, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x6C, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x6C, 2)] = &mc_jmp_ind_c2_target_lo,
        [IDX(qe6502_model_nmos, 0x6C, 3)] = &mc_latch_data_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x6C, 4)] = &mc_jmp_ind_c4_fetch,
        [IDX(qe6502_model_nmos, 0x6C, 5)] = &mc_dispatch,

        /* 0x6D ADC abs ; class=r_abs */
        [IDX(qe6502_model_nmos, 0x6D, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x6D, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x6D, 2)] = &mc_read_addr_with_bus_hi_prefetch,
        [IDX(qe6502_model_nmos, 0x6D, 3)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x6D, 4)] = &mc_dispatch,

        /* 0x6E ROR abs ; class=rw_abs */
        [IDX(qe6502_model_nmos, 0x6E, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x6E, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x6E, 2)] = &mc_latch_addr1_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x6E, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x6E, 4)] = &op_ror_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x6E, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x6E, 6)] = &mc_dispatch,

        /* 0x6F RRA abs ; class=rw_abs */
        [IDX(qe6502_model_nmos, 0x6F, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x6F, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x6F, 2)] = &mc_latch_addr1_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x6F, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x6F, 4)] = &op_rra_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x6F, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x6F, 6)] = &mc_dispatch,

        /* 0x70 BVS rel ; class=branch_rel */
        [IDX(qe6502_model_nmos, 0x70, 0)] = &op_bvs_branch_c0_offset_prefetch,
        [IDX(qe6502_model_nmos, 0x70, 1)] = &mc_branch_rel_c1_apply,
        [IDX(qe6502_model_nmos, 0x70, 2)] = &mc_branch_pgfix_prefetch,
        [IDX(qe6502_model_nmos, 0x70, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x70, 4)] = &mc_dispatch,

        /* 0x71 ADC (zp),Y ; class=r_izy */
        [IDX(qe6502_model_nmos, 0x71, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x71, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x71, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0x71, 3)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x71, 4)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x71, 5)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x71, 6)] = &mc_dispatch,

        /* 0x72 KIL/JAM ; class=kil_jam */
        [IDX(qe6502_model_nmos, 0x72, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x72, 1)] = &mc_kil_jam_read_ffff,
        [IDX(qe6502_model_nmos, 0x72, 2)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x72, 3)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x72, 4)] = &op_kil_jam_r_ffff_pending_data_loop,

        /* 0x73 RRA (zp),Y ; class=rw_izy */
        [IDX(qe6502_model_nmos, 0x73, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x73, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x73, 2)] = &mc_w_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0x73, 3)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x73, 4)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x73, 5)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x73, 6)] = &op_rra_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x73, 7)] = &mc_fetch_illegal_ext_dispatch,

        /* 0x74 NOP zp,X ; class=nop_zpx_read */
        [IDX(qe6502_model_nmos, 0x74, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x74, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x74, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x74, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x74, 4)] = &mc_dispatch,

        /* 0x75 ADC zp,X ; class=r_zpx */
        [IDX(qe6502_model_nmos, 0x75, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x75, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x75, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x75, 3)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x75, 4)] = &mc_dispatch,

        /* 0x76 ROR zp,X ; class=rw_zpx */
        [IDX(qe6502_model_nmos, 0x76, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x76, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x76, 2)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x76, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x76, 4)] = &op_ror_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x76, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x76, 6)] = &mc_dispatch,

        /* 0x77 RRA zp,X ; class=rw_zpx */
        [IDX(qe6502_model_nmos, 0x77, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x77, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0x77, 2)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x77, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x77, 4)] = &op_rra_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x77, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x77, 6)] = &mc_dispatch,

        /* 0x78 SEI ; class=imp */
        [IDX(qe6502_model_nmos, 0x78, 0)] = &op_sei_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0x78, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x78, 2)] = &mc_dispatch,

        /* 0x79 ADC abs,Y ; class=r_aby */
        [IDX(qe6502_model_nmos, 0x79, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x79, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x79, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x79, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x79, 4)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x79, 5)] = &mc_dispatch,

        /* 0x7A NOP ; class=nop_imp */
        [IDX(qe6502_model_nmos, 0x7A, 0)] = &mc_read_pc_prefetch,
        [IDX(qe6502_model_nmos, 0x7A, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x7A, 2)] = &mc_dispatch,

        /* 0x7B RRA abs,Y ; class=rw_aby */
        [IDX(qe6502_model_nmos, 0x7B, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x7B, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x7B, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x7B, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x7B, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x7B, 5)] = &op_rra_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x7B, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x7B, 7)] = &mc_dispatch,

        /* 0x7C NOP abs,X ; class=nop_abx_read */
        [IDX(qe6502_model_nmos, 0x7C, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x7C, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x7C, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x7C, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x7C, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x7C, 5)] = &mc_dispatch,

        /* 0x7D ADC abs,X ; class=r_abx */
        [IDX(qe6502_model_nmos, 0x7D, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x7D, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x7D, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0x7D, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0x7D, 4)] = &op_adc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x7D, 5)] = &mc_dispatch,

        /* 0x7E ROR abs,X ; class=rw_abx */
        [IDX(qe6502_model_nmos, 0x7E, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x7E, 1)] = &mc_rw_abx_c1_hi,
        [IDX(qe6502_model_nmos, 0x7E, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x7E, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x7E, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x7E, 5)] = &op_ror_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x7E, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x7E, 7)] = &mc_dispatch,

        /* 0x7F RRA abs,X ; class=rw_abx */
        [IDX(qe6502_model_nmos, 0x7F, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x7F, 1)] = &mc_rw_abx_c1_hi,
        [IDX(qe6502_model_nmos, 0x7F, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x7F, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x7F, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0x7F, 5)] = &op_rra_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x7F, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x7F, 7)] = &mc_dispatch,

        /* 0x80 NOP #imm ; class=nop_imm */
        [IDX(qe6502_model_nmos, 0x80, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0x80, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x80, 2)] = &mc_dispatch,

        /* 0x81 STA (zp,X) ; class=w_izx */
        [IDX(qe6502_model_nmos, 0x81, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x81, 1)] = &mc_read_zp_latch_x,
        [IDX(qe6502_model_nmos, 0x81, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0x81, 3)] = &mc_w_izx_c3_ptrhi,
        [IDX(qe6502_model_nmos, 0x81, 4)] = &op_sta_w_ready_addrlo_pending_addrhi_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x81, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x81, 6)] = &mc_dispatch,

        /* 0x82 NOP #imm ; class=nop_imm */
        [IDX(qe6502_model_nmos, 0x82, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0x82, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x82, 2)] = &mc_dispatch,

        /* 0x83 SAX (zp,X) ; class=w_izx */
        [IDX(qe6502_model_nmos, 0x83, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x83, 1)] = &mc_read_zp_latch_x,
        [IDX(qe6502_model_nmos, 0x83, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0x83, 3)] = &mc_w_izx_c3_ptrhi,
        [IDX(qe6502_model_nmos, 0x83, 4)] = &op_sax_w_ready_addrlo_pending_addrhi_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x83, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x83, 6)] = &mc_dispatch,

        /* 0x84 STY zp ; class=w_zp */
        [IDX(qe6502_model_nmos, 0x84, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x84, 1)] = &op_sty_w_ready_none_pending_addrlo_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x84, 2)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x84, 3)] = &mc_dispatch,

        /* 0x85 STA zp ; class=w_zp */
        [IDX(qe6502_model_nmos, 0x85, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x85, 1)] = &op_sta_w_ready_none_pending_addrlo_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x85, 2)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x85, 3)] = &mc_dispatch,

        /* 0x86 STX zp ; class=w_zp */
        [IDX(qe6502_model_nmos, 0x86, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x86, 1)] = &op_stx_w_ready_none_pending_addrlo_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x86, 2)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x86, 3)] = &mc_dispatch,

        /* 0x87 SAX zp ; class=w_zp */
        [IDX(qe6502_model_nmos, 0x87, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x87, 1)] = &op_sax_w_ready_none_pending_addrlo_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x87, 2)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x87, 3)] = &mc_dispatch,

        /* 0x88 DEY ; class=imp */
        [IDX(qe6502_model_nmos, 0x88, 0)] = &op_dey_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0x88, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x88, 2)] = &mc_dispatch,

        /* 0x89 NOP #imm ; class=nop_imm */
        [IDX(qe6502_model_nmos, 0x89, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0x89, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x89, 2)] = &mc_dispatch,

        /* 0x8A TXA ; class=imp */
        [IDX(qe6502_model_nmos, 0x8A, 0)] = &op_txa_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0x8A, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x8A, 2)] = &mc_dispatch,

        /* 0x8B XAA #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0x8B, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0x8B, 1)] = &op_xaa_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0x8B, 2)] = &mc_dispatch,

        /* 0x8C STY abs ; class=w_abs */
        [IDX(qe6502_model_nmos, 0x8C, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x8C, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x8C, 2)] = &op_sty_w_ready_addrlo_pending_addrhi_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x8C, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x8C, 4)] = &mc_dispatch,

        /* 0x8D STA abs ; class=w_abs */
        [IDX(qe6502_model_nmos, 0x8D, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x8D, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x8D, 2)] = &op_sta_w_ready_addrlo_pending_addrhi_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x8D, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x8D, 4)] = &mc_dispatch,

        /* 0x8E STX abs ; class=w_abs */
        [IDX(qe6502_model_nmos, 0x8E, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x8E, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x8E, 2)] = &op_stx_w_ready_addrlo_pending_addrhi_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x8E, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x8E, 4)] = &mc_dispatch,

        /* 0x8F SAX abs ; class=w_abs */
        [IDX(qe6502_model_nmos, 0x8F, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x8F, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x8F, 2)] = &op_sax_w_ready_addrlo_pending_addrhi_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x8F, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x8F, 4)] = &mc_dispatch,

        /* 0x90 BCC rel ; class=branch_rel */
        [IDX(qe6502_model_nmos, 0x90, 0)] = &op_bcc_branch_c0_offset_prefetch,
        [IDX(qe6502_model_nmos, 0x90, 1)] = &mc_branch_rel_c1_apply,
        [IDX(qe6502_model_nmos, 0x90, 2)] = &mc_branch_pgfix_prefetch,
        [IDX(qe6502_model_nmos, 0x90, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x90, 4)] = &mc_dispatch,

        /* 0x91 STA (zp),Y ; class=w_izy */
        [IDX(qe6502_model_nmos, 0x91, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x91, 1)] = &mc_latch_zp_addr_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x91, 2)] = &mc_w_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0x91, 3)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x91, 4)] = &op_sta_w_ready_addr_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x91, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x91, 6)] = &mc_dispatch,

        /* 0x92 KIL/JAM ; class=kil_jam */
        [IDX(qe6502_model_nmos, 0x92, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0x92, 1)] = &mc_kil_jam_read_ffff,
        [IDX(qe6502_model_nmos, 0x92, 2)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x92, 3)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0x92, 4)] = &op_kil_jam_r_ffff_pending_data_loop,

        /* 0x93 AHX (zp),Y ; class=unstable_w_izy */
        [IDX(qe6502_model_nmos, 0x93, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x93, 1)] = &mc_latch_zp_addr_read_latch_addr,
        [IDX(qe6502_model_nmos, 0x93, 2)] = &mc_w_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0x93, 3)] = &mc_unstable_store_indexed_probe_preserve_base_high,
        [IDX(qe6502_model_nmos, 0x93, 4)] = &op_ahx_unstable_w_ready_addr_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x93, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x93, 6)] = &mc_dispatch,

        /* 0x94 STY zp,X ; class=w_zpx */
        [IDX(qe6502_model_nmos, 0x94, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x94, 1)] = &mc_read_zp_latch_x,
        [IDX(qe6502_model_nmos, 0x94, 2)] = &op_sty_w_ready_addr_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x94, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x94, 4)] = &mc_dispatch,

        /* 0x95 STA zp,X ; class=w_zpx */
        [IDX(qe6502_model_nmos, 0x95, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x95, 1)] = &mc_read_zp_latch_x,
        [IDX(qe6502_model_nmos, 0x95, 2)] = &op_sta_w_ready_addr_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x95, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x95, 4)] = &mc_dispatch,

        /* 0x96 STX zp,Y ; class=w_zpy */
        [IDX(qe6502_model_nmos, 0x96, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x96, 1)] = &mc_w_zpy_c1_idx,
        [IDX(qe6502_model_nmos, 0x96, 2)] = &op_stx_w_ready_addr_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x96, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x96, 4)] = &mc_dispatch,

        /* 0x97 SAX zp,Y ; class=w_zpy */
        [IDX(qe6502_model_nmos, 0x97, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x97, 1)] = &mc_w_zpy_c1_idx,
        [IDX(qe6502_model_nmos, 0x97, 2)] = &op_sax_w_ready_addr_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x97, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x97, 4)] = &mc_dispatch,

        /* 0x98 TYA ; class=imp */
        [IDX(qe6502_model_nmos, 0x98, 0)] = &op_tya_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0x98, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x98, 2)] = &mc_dispatch,

        /* 0x99 STA abs,Y ; class=w_aby */
        [IDX(qe6502_model_nmos, 0x99, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x99, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x99, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x99, 3)] = &op_sta_w_ready_addr_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x99, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x99, 5)] = &mc_dispatch,

        /* 0x9A TXS ; class=imp */
        [IDX(qe6502_model_nmos, 0x9A, 0)] = &op_txs_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0x9A, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x9A, 2)] = &mc_dispatch,

        /* 0x9B TAS abs,Y ; class=unstable_w_aby */
        [IDX(qe6502_model_nmos, 0x9B, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x9B, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x9B, 2)] = &mc_unstable_store_indexed_probe_preserve_base_high,
        [IDX(qe6502_model_nmos, 0x9B, 3)] = &op_tas_unstable_w_ready_addr_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x9B, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x9B, 5)] = &mc_dispatch,

        /* 0x9C SHY abs,X ; class=unstable_w_abx */
        [IDX(qe6502_model_nmos, 0x9C, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x9C, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x9C, 2)] = &mc_unstable_store_indexed_probe_preserve_base_high,
        [IDX(qe6502_model_nmos, 0x9C, 3)] = &op_shy_unstable_w_ready_addr_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x9C, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x9C, 5)] = &mc_dispatch,

        /* 0x9D STA abs,X ; class=w_abx */
        [IDX(qe6502_model_nmos, 0x9D, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x9D, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x9D, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0x9D, 3)] = &op_sta_w_ready_addr_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x9D, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x9D, 5)] = &mc_dispatch,

        /* 0x9E SHX abs,Y ; class=unstable_w_aby */
        [IDX(qe6502_model_nmos, 0x9E, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x9E, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x9E, 2)] = &mc_unstable_store_indexed_probe_preserve_base_high,
        [IDX(qe6502_model_nmos, 0x9E, 3)] = &op_shx_unstable_w_ready_addr_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x9E, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x9E, 5)] = &mc_dispatch,

        /* 0x9F AHX abs,Y ; class=unstable_w_aby */
        [IDX(qe6502_model_nmos, 0x9F, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x9F, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0x9F, 2)] = &mc_unstable_store_indexed_probe_preserve_base_high,
        [IDX(qe6502_model_nmos, 0x9F, 3)] = &op_ahx_unstable_w_ready_addr_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0x9F, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0x9F, 5)] = &mc_dispatch,

        /* 0xA0 LDY #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0xA0, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0xA0, 1)] = &mc_load_y_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xA0, 2)] = &mc_dispatch,

        /* 0xA1 LDA (zp,X) ; class=r_izx */
        [IDX(qe6502_model_nmos, 0xA1, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xA1, 1)] = &mc_r_izx_c1_idx,
        [IDX(qe6502_model_nmos, 0xA1, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0xA1, 3)] = &mc_latch_data_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xA1, 4)] = &mc_r_izx_c4_data_no_latch_prefetch,
        [IDX(qe6502_model_nmos, 0xA1, 5)] = &mc_load_a_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xA1, 6)] = &mc_dispatch,

        /* 0xA2 LDX #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0xA2, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0xA2, 1)] = &mc_load_x_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xA2, 2)] = &mc_dispatch,

        /* 0xA3 LAX (zp,X) ; class=r_izx */
        [IDX(qe6502_model_nmos, 0xA3, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xA3, 1)] = &mc_r_izx_c1_idx,
        [IDX(qe6502_model_nmos, 0xA3, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0xA3, 3)] = &mc_latch_data_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xA3, 4)] = &mc_r_izx_c4_data_no_latch_prefetch,
        [IDX(qe6502_model_nmos, 0xA3, 5)] = &op_lax_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xA3, 6)] = &mc_dispatch,

        /* 0xA4 LDY zp ; class=r_zp */
        [IDX(qe6502_model_nmos, 0xA4, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xA4, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0xA4, 2)] = &mc_load_y_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xA4, 3)] = &mc_dispatch,

        /* 0xA5 LDA zp ; class=r_zp */
        [IDX(qe6502_model_nmos, 0xA5, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xA5, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0xA5, 2)] = &mc_load_a_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xA5, 3)] = &mc_dispatch,

        /* 0xA6 LDX zp ; class=r_zp */
        [IDX(qe6502_model_nmos, 0xA6, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xA6, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0xA6, 2)] = &mc_load_x_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xA6, 3)] = &mc_dispatch,

        /* 0xA7 LAX zp ; class=r_zp */
        [IDX(qe6502_model_nmos, 0xA7, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xA7, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0xA7, 2)] = &op_lax_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xA7, 3)] = &mc_dispatch,

        /* 0xA8 TAY ; class=imp */
        [IDX(qe6502_model_nmos, 0xA8, 0)] = &op_tay_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0xA8, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xA8, 2)] = &mc_dispatch,

        /* 0xA9 LDA #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0xA9, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0xA9, 1)] = &mc_load_a_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xA9, 2)] = &mc_dispatch,

        /* 0xAA TAX ; class=imp */
        [IDX(qe6502_model_nmos, 0xAA, 0)] = &op_tax_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0xAA, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xAA, 2)] = &mc_dispatch,

        /* 0xAB LXA #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0xAB, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0xAB, 1)] = &op_lxa_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xAB, 2)] = &mc_dispatch,

        /* 0xAC LDY abs ; class=r_abs */
        [IDX(qe6502_model_nmos, 0xAC, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xAC, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xAC, 2)] = &mc_read_addr_with_bus_hi_prefetch,
        [IDX(qe6502_model_nmos, 0xAC, 3)] = &mc_load_y_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xAC, 4)] = &mc_dispatch,

        /* 0xAD LDA abs ; class=r_abs */
        [IDX(qe6502_model_nmos, 0xAD, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xAD, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xAD, 2)] = &mc_read_addr_with_bus_hi_prefetch,
        [IDX(qe6502_model_nmos, 0xAD, 3)] = &mc_load_a_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xAD, 4)] = &mc_dispatch,

        /* 0xAE LDX abs ; class=r_abs */
        [IDX(qe6502_model_nmos, 0xAE, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xAE, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xAE, 2)] = &mc_read_addr_with_bus_hi_prefetch,
        [IDX(qe6502_model_nmos, 0xAE, 3)] = &mc_load_x_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xAE, 4)] = &mc_dispatch,

        /* 0xAF LAX abs ; class=r_abs */
        [IDX(qe6502_model_nmos, 0xAF, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xAF, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xAF, 2)] = &mc_read_addr_with_bus_hi_prefetch,
        [IDX(qe6502_model_nmos, 0xAF, 3)] = &op_lax_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xAF, 4)] = &mc_dispatch,

        /* 0xB0 BCS rel ; class=branch_rel */
        [IDX(qe6502_model_nmos, 0xB0, 0)] = &op_bcs_branch_c0_offset_prefetch,
        [IDX(qe6502_model_nmos, 0xB0, 1)] = &mc_branch_rel_c1_apply,
        [IDX(qe6502_model_nmos, 0xB0, 2)] = &mc_branch_pgfix_prefetch,
        [IDX(qe6502_model_nmos, 0xB0, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xB0, 4)] = &mc_dispatch,

        /* 0xB1 LDA (zp),Y ; class=r_izy */
        [IDX(qe6502_model_nmos, 0xB1, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xB1, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xB1, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0xB1, 3)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xB1, 4)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xB1, 5)] = &mc_load_a_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xB1, 6)] = &mc_dispatch,

        /* 0xB2 KIL/JAM ; class=kil_jam */
        [IDX(qe6502_model_nmos, 0xB2, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0xB2, 1)] = &mc_kil_jam_read_ffff,
        [IDX(qe6502_model_nmos, 0xB2, 2)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0xB2, 3)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0xB2, 4)] = &op_kil_jam_r_ffff_pending_data_loop,

        /* 0xB3 LAX (zp),Y ; class=r_izy */
        [IDX(qe6502_model_nmos, 0xB3, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xB3, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xB3, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0xB3, 3)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xB3, 4)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xB3, 5)] = &op_lax_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xB3, 6)] = &mc_dispatch,

        /* 0xB4 LDY zp,X ; class=r_zpx */
        [IDX(qe6502_model_nmos, 0xB4, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xB4, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0xB4, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xB4, 3)] = &mc_load_y_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xB4, 4)] = &mc_dispatch,

        /* 0xB5 LDA zp,X ; class=r_zpx */
        [IDX(qe6502_model_nmos, 0xB5, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xB5, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0xB5, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xB5, 3)] = &mc_load_a_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xB5, 4)] = &mc_dispatch,

        /* 0xB6 LDX zp,Y ; class=r_zpy */
        [IDX(qe6502_model_nmos, 0xB6, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xB6, 1)] = &mc_r_zpy_c1_idx,
        [IDX(qe6502_model_nmos, 0xB6, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xB6, 3)] = &mc_load_x_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xB6, 4)] = &mc_dispatch,

        /* 0xB7 LAX zp,Y ; class=r_zpy */
        [IDX(qe6502_model_nmos, 0xB7, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xB7, 1)] = &mc_r_zpy_c1_idx,
        [IDX(qe6502_model_nmos, 0xB7, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xB7, 3)] = &op_lax_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xB7, 4)] = &mc_dispatch,

        /* 0xB8 CLV ; class=imp */
        [IDX(qe6502_model_nmos, 0xB8, 0)] = &op_clv_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0xB8, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xB8, 2)] = &mc_dispatch,

        /* 0xB9 LDA abs,Y ; class=r_aby */
        [IDX(qe6502_model_nmos, 0xB9, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xB9, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xB9, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xB9, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xB9, 4)] = &mc_load_a_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xB9, 5)] = &mc_dispatch,

        /* 0xBA TSX ; class=imp */
        [IDX(qe6502_model_nmos, 0xBA, 0)] = &op_tsx_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0xBA, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xBA, 2)] = &mc_dispatch,

        /* 0xBB LAS abs,Y ; class=r_aby */
        [IDX(qe6502_model_nmos, 0xBB, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xBB, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xBB, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xBB, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xBB, 4)] = &op_las_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xBB, 5)] = &mc_dispatch,

        /* 0xBC LDY abs,X ; class=r_abx */
        [IDX(qe6502_model_nmos, 0xBC, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xBC, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xBC, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xBC, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xBC, 4)] = &mc_load_y_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xBC, 5)] = &mc_dispatch,

        /* 0xBD LDA abs,X ; class=r_abx */
        [IDX(qe6502_model_nmos, 0xBD, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xBD, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xBD, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xBD, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xBD, 4)] = &mc_load_a_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xBD, 5)] = &mc_dispatch,

        /* 0xBE LDX abs,Y ; class=r_aby */
        [IDX(qe6502_model_nmos, 0xBE, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xBE, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xBE, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xBE, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xBE, 4)] = &mc_load_x_update_nz_fetch,
        [IDX(qe6502_model_nmos, 0xBE, 5)] = &mc_dispatch,

        /* 0xBF LAX abs,Y ; class=r_aby */
        [IDX(qe6502_model_nmos, 0xBF, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xBF, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xBF, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xBF, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xBF, 4)] = &op_lax_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xBF, 5)] = &mc_dispatch,

        /* 0xC0 CPY #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0xC0, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0xC0, 1)] = &op_cpy_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xC0, 2)] = &mc_dispatch,

        /* 0xC1 CMP (zp,X) ; class=r_izx */
        [IDX(qe6502_model_nmos, 0xC1, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xC1, 1)] = &mc_r_izx_c1_idx,
        [IDX(qe6502_model_nmos, 0xC1, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0xC1, 3)] = &mc_latch_data_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xC1, 4)] = &mc_r_izx_c4_data_no_latch_prefetch,
        [IDX(qe6502_model_nmos, 0xC1, 5)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xC1, 6)] = &mc_dispatch,

        /* 0xC2 NOP #imm ; class=nop_imm */
        [IDX(qe6502_model_nmos, 0xC2, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0xC2, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xC2, 2)] = &mc_dispatch,

        /* 0xC3 DCP (zp,X) ; class=rw_izx */
        [IDX(qe6502_model_nmos, 0xC3, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xC3, 1)] = &mc_r_izx_c1_idx,
        [IDX(qe6502_model_nmos, 0xC3, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0xC3, 3)] = &mc_latch_data_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xC3, 4)] = &mc_r_izx_c4_data,
        [IDX(qe6502_model_nmos, 0xC3, 5)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xC3, 6)] = &op_dcp_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xC3, 7)] = &mc_fetch_illegal_ext_dispatch,

        /* 0xC4 CPY zp ; class=r_zp */
        [IDX(qe6502_model_nmos, 0xC4, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xC4, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0xC4, 2)] = &op_cpy_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xC4, 3)] = &mc_dispatch,

        /* 0xC5 CMP zp ; class=r_zp */
        [IDX(qe6502_model_nmos, 0xC5, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xC5, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0xC5, 2)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xC5, 3)] = &mc_dispatch,

        /* 0xC6 DEC zp ; class=rw_zp */
        [IDX(qe6502_model_nmos, 0xC6, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xC6, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xC6, 2)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xC6, 3)] = &op_dec_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xC6, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xC6, 5)] = &mc_dispatch,

        /* 0xC7 DCP zp ; class=rw_zp */
        [IDX(qe6502_model_nmos, 0xC7, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xC7, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xC7, 2)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xC7, 3)] = &op_dcp_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xC7, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xC7, 5)] = &mc_dispatch,

        /* 0xC8 INY ; class=imp */
        [IDX(qe6502_model_nmos, 0xC8, 0)] = &op_iny_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0xC8, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xC8, 2)] = &mc_dispatch,

        /* 0xC9 CMP #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0xC9, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0xC9, 1)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xC9, 2)] = &mc_dispatch,

        /* 0xCA DEX ; class=imp */
        [IDX(qe6502_model_nmos, 0xCA, 0)] = &op_dex_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0xCA, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xCA, 2)] = &mc_dispatch,

        /* 0xCB AXS #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0xCB, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0xCB, 1)] = &op_axs_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xCB, 2)] = &mc_dispatch,


        /* 0xCC CPY abs ; class=r_abs */
        [IDX(qe6502_model_nmos, 0xCC, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xCC, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xCC, 2)] = &mc_read_addr_with_bus_hi_prefetch,
        [IDX(qe6502_model_nmos, 0xCC, 3)] = &op_cpy_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xCC, 4)] = &mc_dispatch,

        /* 0xCD CMP abs ; class=r_abs */
        [IDX(qe6502_model_nmos, 0xCD, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xCD, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xCD, 2)] = &mc_read_addr_with_bus_hi_prefetch,
        [IDX(qe6502_model_nmos, 0xCD, 3)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xCD, 4)] = &mc_dispatch,

        /* 0xCE DEC abs ; class=rw_abs */
        [IDX(qe6502_model_nmos, 0xCE, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xCE, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xCE, 2)] = &mc_latch_addr1_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xCE, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xCE, 4)] = &op_dec_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xCE, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xCE, 6)] = &mc_dispatch,

        /* 0xCF DCP abs ; class=rw_abs */
        [IDX(qe6502_model_nmos, 0xCF, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xCF, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xCF, 2)] = &mc_latch_addr1_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xCF, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xCF, 4)] = &op_dcp_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xCF, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xCF, 6)] = &mc_dispatch,

        /* 0xD0 BNE rel ; class=branch_rel */
        [IDX(qe6502_model_nmos, 0xD0, 0)] = &op_bne_branch_c0_offset_prefetch,
        [IDX(qe6502_model_nmos, 0xD0, 1)] = &mc_branch_rel_c1_apply,
        [IDX(qe6502_model_nmos, 0xD0, 2)] = &mc_branch_pgfix_prefetch,
        [IDX(qe6502_model_nmos, 0xD0, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xD0, 4)] = &mc_dispatch,

        /* 0xD1 CMP (zp),Y ; class=r_izy */
        [IDX(qe6502_model_nmos, 0xD1, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xD1, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xD1, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0xD1, 3)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xD1, 4)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xD1, 5)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xD1, 6)] = &mc_dispatch,

        /* 0xD2 KIL/JAM ; class=kil_jam */
        [IDX(qe6502_model_nmos, 0xD2, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0xD2, 1)] = &mc_kil_jam_read_ffff,
        [IDX(qe6502_model_nmos, 0xD2, 2)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0xD2, 3)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0xD2, 4)] = &op_kil_jam_r_ffff_pending_data_loop,

        /* 0xD3 DCP (zp),Y ; class=rw_izy */
        [IDX(qe6502_model_nmos, 0xD3, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xD3, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xD3, 2)] = &mc_w_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0xD3, 3)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0xD3, 4)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xD3, 5)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xD3, 6)] = &op_dcp_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xD3, 7)] = &mc_fetch_illegal_ext_dispatch,

        /* 0xD4 NOP zp,X ; class=nop_zpx_read */
        [IDX(qe6502_model_nmos, 0xD4, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xD4, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0xD4, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xD4, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xD4, 4)] = &mc_dispatch,

        /* 0xD5 CMP zp,X ; class=r_zpx */
        [IDX(qe6502_model_nmos, 0xD5, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xD5, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0xD5, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xD5, 3)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xD5, 4)] = &mc_dispatch,

        /* 0xD6 DEC zp,X ; class=rw_zpx */
        [IDX(qe6502_model_nmos, 0xD6, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xD6, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0xD6, 2)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xD6, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xD6, 4)] = &op_dec_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xD6, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xD6, 6)] = &mc_dispatch,

        /* 0xD7 DCP zp,X ; class=rw_zpx */
        [IDX(qe6502_model_nmos, 0xD7, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xD7, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0xD7, 2)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xD7, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xD7, 4)] = &op_dcp_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xD7, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xD7, 6)] = &mc_dispatch,

        /* 0xD8 CLD ; class=imp */
        [IDX(qe6502_model_nmos, 0xD8, 0)] = &op_cld_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0xD8, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xD8, 2)] = &mc_dispatch,

        /* 0xD9 CMP abs,Y ; class=r_aby */
        [IDX(qe6502_model_nmos, 0xD9, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xD9, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xD9, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xD9, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xD9, 4)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xD9, 5)] = &mc_dispatch,

        /* 0xDA NOP ; class=nop_imp */
        [IDX(qe6502_model_nmos, 0xDA, 0)] = &mc_read_pc_prefetch,
        [IDX(qe6502_model_nmos, 0xDA, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xDA, 2)] = &mc_dispatch,

        /* 0xDB DCP abs,Y ; class=rw_aby */
        [IDX(qe6502_model_nmos, 0xDB, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xDB, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xDB, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0xDB, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xDB, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xDB, 5)] = &op_dcp_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xDB, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xDB, 7)] = &mc_dispatch,

        /* 0xDC NOP abs,X ; class=nop_abx_read */
        [IDX(qe6502_model_nmos, 0xDC, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xDC, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xDC, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xDC, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xDC, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xDC, 5)] = &mc_dispatch,

        /* 0xDD CMP abs,X ; class=r_abx */
        [IDX(qe6502_model_nmos, 0xDD, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xDD, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xDD, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xDD, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xDD, 4)] = &op_cmp_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xDD, 5)] = &mc_dispatch,

        /* 0xDE DEC abs,X ; class=rw_abx */
        [IDX(qe6502_model_nmos, 0xDE, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xDE, 1)] = &mc_rw_abx_c1_hi,
        [IDX(qe6502_model_nmos, 0xDE, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0xDE, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xDE, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xDE, 5)] = &op_dec_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xDE, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xDE, 7)] = &mc_dispatch,

        /* 0xDF DCP abs,X ; class=rw_abx */
        [IDX(qe6502_model_nmos, 0xDF, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xDF, 1)] = &mc_rw_abx_c1_hi,
        [IDX(qe6502_model_nmos, 0xDF, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0xDF, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xDF, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xDF, 5)] = &op_dcp_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xDF, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xDF, 7)] = &mc_dispatch,

        /* 0xE0 CPX #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0xE0, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0xE0, 1)] = &op_cpx_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xE0, 2)] = &mc_dispatch,

        /* 0xE1 SBC (zp,X) ; class=r_izx */
        [IDX(qe6502_model_nmos, 0xE1, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xE1, 1)] = &mc_r_izx_c1_idx,
        [IDX(qe6502_model_nmos, 0xE1, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0xE1, 3)] = &mc_latch_data_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xE1, 4)] = &mc_r_izx_c4_data_no_latch_prefetch,
        [IDX(qe6502_model_nmos, 0xE1, 5)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xE1, 6)] = &mc_dispatch,

        /* 0xE2 NOP #imm ; class=nop_imm */
        [IDX(qe6502_model_nmos, 0xE2, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0xE2, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xE2, 2)] = &mc_dispatch,

        /* 0xE3 ISC (zp,X) ; class=rw_izx */
        [IDX(qe6502_model_nmos, 0xE3, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xE3, 1)] = &mc_r_izx_c1_idx,
        [IDX(qe6502_model_nmos, 0xE3, 2)] = &mc_izx_read_ptrlo_inc_ptr,
        [IDX(qe6502_model_nmos, 0xE3, 3)] = &mc_latch_data_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xE3, 4)] = &mc_r_izx_c4_data,
        [IDX(qe6502_model_nmos, 0xE3, 5)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xE3, 6)] = &op_isc_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xE3, 7)] = &mc_fetch_illegal_ext_dispatch,

        /* 0xE4 CPX zp ; class=r_zp */
        [IDX(qe6502_model_nmos, 0xE4, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xE4, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0xE4, 2)] = &op_cpx_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xE4, 3)] = &mc_dispatch,

        /* 0xE5 SBC zp ; class=r_zp */
        [IDX(qe6502_model_nmos, 0xE5, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xE5, 1)] = &mc_r_zp_c1_data_prefetch,
        [IDX(qe6502_model_nmos, 0xE5, 2)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xE5, 3)] = &mc_dispatch,

        /* 0xE6 INC zp ; class=rw_zp */
        [IDX(qe6502_model_nmos, 0xE6, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xE6, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xE6, 2)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xE6, 3)] = &op_inc_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xE6, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xE6, 5)] = &mc_dispatch,

        /* 0xE7 ISC zp ; class=rw_zp */
        [IDX(qe6502_model_nmos, 0xE7, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xE7, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xE7, 2)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xE7, 3)] = &op_isc_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xE7, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xE7, 5)] = &mc_dispatch,

        /* 0xE8 INX ; class=imp */
        [IDX(qe6502_model_nmos, 0xE8, 0)] = &op_inx_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0xE8, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xE8, 2)] = &mc_dispatch,

        /* 0xE9 SBC #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0xE9, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0xE9, 1)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xE9, 2)] = &mc_dispatch,

        /* 0xEA NOP ; class=imp */
        [IDX(qe6502_model_nmos, 0xEA, 0)] = &mc_read_pc_prefetch,
        [IDX(qe6502_model_nmos, 0xEA, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xEA, 2)] = &mc_dispatch,

        /* 0xEB SBC #imm ; class=r_imm */
        [IDX(qe6502_model_nmos, 0xEB, 0)] = &mc_read_pc_inc_prefetch,
        [IDX(qe6502_model_nmos, 0xEB, 1)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xEB, 2)] = &mc_dispatch,


        /* 0xEC CPX abs ; class=r_abs */
        [IDX(qe6502_model_nmos, 0xEC, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xEC, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xEC, 2)] = &mc_read_addr_with_bus_hi_prefetch,
        [IDX(qe6502_model_nmos, 0xEC, 3)] = &op_cpx_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xEC, 4)] = &mc_dispatch,

        /* 0xED SBC abs ; class=r_abs */
        [IDX(qe6502_model_nmos, 0xED, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xED, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xED, 2)] = &mc_read_addr_with_bus_hi_prefetch,
        [IDX(qe6502_model_nmos, 0xED, 3)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xED, 4)] = &mc_dispatch,

        /* 0xEE INC abs ; class=rw_abs */
        [IDX(qe6502_model_nmos, 0xEE, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xEE, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xEE, 2)] = &mc_latch_addr1_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xEE, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xEE, 4)] = &op_inc_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xEE, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xEE, 6)] = &mc_dispatch,

        /* 0xEF ISC abs ; class=rw_abs */
        [IDX(qe6502_model_nmos, 0xEF, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xEF, 1)] = &mc_latch_addr0_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xEF, 2)] = &mc_latch_addr1_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xEF, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xEF, 4)] = &op_isc_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xEF, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xEF, 6)] = &mc_dispatch,

        /* 0xF0 BEQ rel ; class=branch_rel */
        [IDX(qe6502_model_nmos, 0xF0, 0)] = &op_beq_branch_c0_offset_prefetch,
        [IDX(qe6502_model_nmos, 0xF0, 1)] = &mc_branch_rel_c1_apply,
        [IDX(qe6502_model_nmos, 0xF0, 2)] = &mc_branch_pgfix_prefetch,
        [IDX(qe6502_model_nmos, 0xF0, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xF0, 4)] = &mc_dispatch,

        /* 0xF1 SBC (zp),Y ; class=r_izy */
        [IDX(qe6502_model_nmos, 0xF1, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xF1, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xF1, 2)] = &mc_r_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0xF1, 3)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xF1, 4)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xF1, 5)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xF1, 6)] = &mc_dispatch,

        /* 0xF2 KIL/JAM ; class=kil_jam */
        [IDX(qe6502_model_nmos, 0xF2, 0)] = &mc_read_pc,
        [IDX(qe6502_model_nmos, 0xF2, 1)] = &mc_kil_jam_read_ffff,
        [IDX(qe6502_model_nmos, 0xF2, 2)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0xF2, 3)] = &mc_read_fffe,
        [IDX(qe6502_model_nmos, 0xF2, 4)] = &op_kil_jam_r_ffff_pending_data_loop,

        /* 0xF3 ISC (zp),Y ; class=rw_izy */
        [IDX(qe6502_model_nmos, 0xF3, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xF3, 1)] = &mc_latch_addr0_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xF3, 2)] = &mc_w_izy_c2_ptrhi,
        [IDX(qe6502_model_nmos, 0xF3, 3)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0xF3, 4)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xF3, 5)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xF3, 6)] = &op_isc_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xF3, 7)] = &mc_fetch_illegal_ext_dispatch,

        /* 0xF4 NOP zp,X ; class=nop_zpx_read */
        [IDX(qe6502_model_nmos, 0xF4, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xF4, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0xF4, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xF4, 3)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xF4, 4)] = &mc_dispatch,

        /* 0xF5 SBC zp,X ; class=r_zpx */
        [IDX(qe6502_model_nmos, 0xF5, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xF5, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0xF5, 2)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xF5, 3)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xF5, 4)] = &mc_dispatch,

        /* 0xF6 INC zp,X ; class=rw_zpx */
        [IDX(qe6502_model_nmos, 0xF6, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xF6, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0xF6, 2)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xF6, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xF6, 4)] = &op_inc_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xF6, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xF6, 6)] = &mc_dispatch,

        /* 0xF7 ISC zp,X ; class=rw_zpx */
        [IDX(qe6502_model_nmos, 0xF7, 0)] = &mc_clear_latch_addr_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xF7, 1)] = &mc_zpx_c1_idx,
        [IDX(qe6502_model_nmos, 0xF7, 2)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xF7, 3)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xF7, 4)] = &op_isc_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xF7, 5)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xF7, 6)] = &mc_dispatch,

        /* 0xF8 SED ; class=imp */
        [IDX(qe6502_model_nmos, 0xF8, 0)] = &op_sed_imp_ready_none_pending_none_dummy_prefetch,
        [IDX(qe6502_model_nmos, 0xF8, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xF8, 2)] = &mc_dispatch,

        /* 0xF9 SBC abs,Y ; class=r_aby */
        [IDX(qe6502_model_nmos, 0xF9, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xF9, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xF9, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xF9, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xF9, 4)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xF9, 5)] = &mc_dispatch,

        /* 0xFA NOP ; class=nop_imp */
        [IDX(qe6502_model_nmos, 0xFA, 0)] = &mc_read_pc_prefetch,
        [IDX(qe6502_model_nmos, 0xFA, 1)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xFA, 2)] = &mc_dispatch,

        /* 0xFB ISC abs,Y ; class=rw_aby */
        [IDX(qe6502_model_nmos, 0xFB, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xFB, 1)] = &mc_latch_aby_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xFB, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0xFB, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xFB, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xFB, 5)] = &op_isc_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xFB, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xFB, 7)] = &mc_dispatch,

        /* 0xFC NOP abs,X ; class=nop_abx_read */
        [IDX(qe6502_model_nmos, 0xFC, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xFC, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xFC, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xFC, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xFC, 4)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xFC, 5)] = &mc_dispatch,

        /* 0xFD SBC abs,X ; class=r_abx */
        [IDX(qe6502_model_nmos, 0xFD, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xFD, 1)] = &mc_latch_abx_base_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xFD, 2)] = &mc_r_indexed_probe_no_latch_prefetch_no_page_cross,
        [IDX(qe6502_model_nmos, 0xFD, 3)] = &mc_read_latch_addr_prefetch,
        [IDX(qe6502_model_nmos, 0xFD, 4)] = &op_sbc_r_ready_none_pending_data_fetch,
        [IDX(qe6502_model_nmos, 0xFD, 5)] = &mc_dispatch,

        /* 0xFE INC abs,X ; class=rw_abx */
        [IDX(qe6502_model_nmos, 0xFE, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xFE, 1)] = &mc_rw_abx_c1_hi,
        [IDX(qe6502_model_nmos, 0xFE, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0xFE, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xFE, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xFE, 5)] = &op_inc_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xFE, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xFE, 7)] = &mc_dispatch,

        /* 0xFF ISC abs,X ; class=rw_abx */
        [IDX(qe6502_model_nmos, 0xFF, 0)] = &mc_read_pc_inc,
        [IDX(qe6502_model_nmos, 0xFF, 1)] = &mc_rw_abx_c1_hi,
        [IDX(qe6502_model_nmos, 0xFF, 2)] = &mc_w_indexed_probe,
        [IDX(qe6502_model_nmos, 0xFF, 3)] = &mc_read_latch_addr,
        [IDX(qe6502_model_nmos, 0xFF, 4)] = &mc_latch_data_write_latch_addr,
        [IDX(qe6502_model_nmos, 0xFF, 5)] = &op_isc_rw_ready_addr_data_pending_none_wr_prefetch,
        [IDX(qe6502_model_nmos, 0xFF, 6)] = &mc_fetch,
        [IDX(qe6502_model_nmos, 0xFF, 7)] = &mc_dispatch

