static void schedule_load_mem_op_8(WDC65816* cpu);
static void load_mem_op_8_1(WDC65816* cpu);

static void schedule_load_mem_op_16(WDC65816* cpu);
static void load_mem_op_16_1(WDC65816* cpu);

static void schedule_rmw_a_8(WDC65816* cpu);
static void rmw_a_8_1(WDC65816* cpu);

static void schedule_rmw_a_16(WDC65816* cpu);
static void rmw_a_16_1(WDC65816* cpu);

static void schedule_rmw_mem_8(WDC65816* cpu);
static void rmw_mem_8_1(WDC65816* cpu);
static void rmw_mem_8_2(WDC65816* cpu);

static void schedule_rmw_mem_16(WDC65816* cpu);
static void rmw_mem_16_1(WDC65816* cpu);
static void rmw_mem_16_2(WDC65816* cpu);


static void schedule_and_8(WDC65816* cpu); 
static void schedule_and_16(WDC65816* cpu);
static void op_and_8(WDC65816* cpu); 
static void op_and_16(WDC65816* cpu); 

static void schedule_bit_8(WDC65816* cpu); 
static void schedule_bit_16(WDC65816* cpu);
static void op_bit_8(WDC65816* cpu); 
static void op_bit_16(WDC65816* cpu); 

static void schedule_cmp_8(WDC65816* cpu); 
static void schedule_cmp_16(WDC65816* cpu);
static void op_cmp_8(WDC65816* cpu); 
static void op_cmp_16(WDC65816* cpu); 

static void schedule_eor_8(WDC65816* cpu); 
static void schedule_eor_16(WDC65816* cpu);
static void op_eor_8(WDC65816* cpu); 
static void op_eor_16(WDC65816* cpu); 

static void schedule_cpx_8(WDC65816* cpu); 
static void schedule_cpx_16(WDC65816* cpu);
static void op_cpx_8(WDC65816* cpu); 
static void op_cpx_16(WDC65816* cpu); 

static void schedule_cpy_8(WDC65816* cpu); 
static void schedule_cpy_16(WDC65816* cpu);
static void op_cpy_8(WDC65816* cpu); 
static void op_cpy_16(WDC65816* cpu); 

static void schedule_lda_8(WDC65816* cpu); 
static void schedule_lda_16(WDC65816* cpu);
static void op_lda_8(WDC65816* cpu); 
static void op_lda_16(WDC65816* cpu); 

static void schedule_ldx_8(WDC65816* cpu); 
static void schedule_ldx_16(WDC65816* cpu);
static void op_ldx_8(WDC65816* cpu); 
static void op_ldx_16(WDC65816* cpu); 

static void schedule_ldy_8(WDC65816* cpu); 
static void schedule_ldy_16(WDC65816* cpu);
static void op_ldy_8(WDC65816* cpu); 
static void op_ldy_16(WDC65816* cpu); 

static void schedule_ora_8(WDC65816* cpu); 
static void schedule_ora_16(WDC65816* cpu);
static void op_ora_8(WDC65816* cpu); 
static void op_ora_16(WDC65816* cpu); 

static void schedule_ora_8(WDC65816* cpu); 
static void schedule_ora_16(WDC65816* cpu);
static void op_ora_8(WDC65816* cpu); 
static void op_ora_16(WDC65816* cpu); 

static void schedule_adc_8(WDC65816* cpu); 
static void schedule_adc_16(WDC65816* cpu);
static void op_adc_8(WDC65816* cpu); 
static void op_adc_16(WDC65816* cpu); 

static void schedule_sbc_8(WDC65816* cpu); 
static void schedule_sbc_16(WDC65816* cpu);
static void op_sbc_8(WDC65816* cpu); 
static void op_sbc_16(WDC65816* cpu); 


static void schedule_asl_a_8(WDC65816* cpu); 
static void schedule_asl_a_16(WDC65816* cpu);
static void schedule_asl_rmw_8(WDC65816* cpu); 
static void schedule_asl_rmw_16(WDC65816* cpu);
static void op_asl_8(WDC65816* cpu); 
static void op_asl_16(WDC65816* cpu); 

static void schedule_dec_a_8(WDC65816* cpu); 
static void schedule_dec_a_16(WDC65816* cpu);
static void schedule_dec_rmw_8(WDC65816* cpu); 
static void schedule_dec_rmw_16(WDC65816* cpu);
static void op_dec_8(WDC65816* cpu); 
static void op_dec_16(WDC65816* cpu); 

static void schedule_inc_a_8(WDC65816* cpu); 
static void schedule_inc_a_16(WDC65816* cpu);
static void schedule_inc_rmw_8(WDC65816* cpu); 
static void schedule_inc_rmw_16(WDC65816* cpu);
static void op_inc_8(WDC65816* cpu); 
static void op_inc_16(WDC65816* cpu); 

static void schedule_rol_a_8(WDC65816* cpu); 
static void schedule_rol_a_16(WDC65816* cpu);
static void schedule_rol_rmw_8(WDC65816* cpu); 
static void schedule_rol_rmw_16(WDC65816* cpu);
static void op_rol_8(WDC65816* cpu); 
static void op_rol_16(WDC65816* cpu); 

static void schedule_ror_a_8(WDC65816* cpu); 
static void schedule_ror_a_16(WDC65816* cpu);
static void schedule_ror_rmw_8(WDC65816* cpu); 
static void schedule_ror_rmw_16(WDC65816* cpu);
static void op_ror_8(WDC65816* cpu); 
static void op_ror_16(WDC65816* cpu); 

static void schedule_trb_rmw_8(WDC65816* cpu); 
static void schedule_trb_rmw_16(WDC65816* cpu);
static void op_trb_8(WDC65816* cpu); 
static void op_trb_16(WDC65816* cpu); 

static void schedule_tsb_rmw_8(WDC65816* cpu); 
static void schedule_tsb_rmw_16(WDC65816* cpu);
static void op_tsb_8(WDC65816* cpu); 
static void op_tsb_16(WDC65816* cpu); 

static void schedule_lsr_a_8(WDC65816* cpu); 
static void schedule_lsr_a_16(WDC65816* cpu);
static void schedule_lsr_rmw_8(WDC65816* cpu); 
static void schedule_lsr_rmw_16(WDC65816* cpu);
static void op_lsr(WDC65816* cpu); 

static void schedule_branch(WDC65816* cpu);
static void branch_1(WDC65816* cpu);
static void branch_2(WDC65816* cpu);

static void schedule_bcc(WDC65816* cpu);
static void schedule_bcs(WDC65816* cpu);
static void schedule_bne(WDC65816* cpu);
static void schedule_beq(WDC65816* cpu);
static void schedule_bpl(WDC65816* cpu);
static void schedule_bmi(WDC65816* cpu);
static void schedule_bvc(WDC65816* cpu);
static void schedule_bvs(WDC65816* cpu);
static void schedule_bra(WDC65816* cpu);

static void schedule_brl(WDC65816* cpu);
static void brl_1(WDC65816* cpu);
static void brl_2(WDC65816* cpu);

static void schedule_interrupt(WDC65816* cpu);
static void interrupt_1(WDC65816* cpu);
static void interrupt_2(WDC65816* cpu);
static void interrupt_3(WDC65816* cpu);
static void interrupt_4(WDC65816* cpu);
static void interrupt_5(WDC65816* cpu);
static void interrupt_6(WDC65816* cpu);
static void interrupt_7(WDC65816* cpu);

static void schedule_brk(WDC65816* cpu);
static void schedule_cop(WDC65816* cpu);

static void schedule_clc(WDC65816* cpu);
static void clc_1(WDC65816* cpu);

static void schedule_cld(WDC65816* cpu);
static void cld_1(WDC65816* cpu);

static void schedule_cli(WDC65816* cpu);
static void cli_1(WDC65816* cpu);

static void schedule_clv(WDC65816* cpu);
static void clv_1(WDC65816* cpu);

static void schedule_dex_8(WDC65816* cpu);
static void schedule_dex_16(WDC65816* cpu);
static void dex_8_1(WDC65816* cpu);
static void dex_16_1(WDC65816* cpu);

static void schedule_dey_8(WDC65816* cpu);
static void schedule_dey_16(WDC65816* cpu);
static void dey_8_1(WDC65816* cpu);
static void dey_16_1(WDC65816* cpu);

static void schedule_inx_8(WDC65816* cpu);
static void schedule_inx_16(WDC65816* cpu);
static void inx_8_1(WDC65816* cpu);
static void inx_16_1(WDC65816* cpu);

static void schedule_iny_8(WDC65816* cpu);
static void schedule_iny_16(WDC65816* cpu);
static void iny_8_1(WDC65816* cpu);
static void iny_16_1(WDC65816* cpu);

static void schedule_jmp_a(WDC65816* cpu);
static void jmp_a_1(WDC65816* cpu);
static void jmp_a_2(WDC65816* cpu);

static void schedule_jmp_a_indr(WDC65816* cpu);
static void jmp_a_indr_1(WDC65816* cpu);
static void jmp_a_indr_2(WDC65816* cpu);
static void jmp_a_indr_3(WDC65816* cpu);
static void jmp_a_indr_4(WDC65816* cpu);

static void schedule_jmp_a_x_indr(WDC65816* cpu);
static void jmp_a_x_indr_1(WDC65816* cpu);
static void jmp_a_x_indr_2(WDC65816* cpu);

static void schedule_jml(WDC65816* cpu);
static void jml_1(WDC65816* cpu);
static void jml_2(WDC65816* cpu);

static void schedule_jml_indr(WDC65816* cpu);
static void jml_indr_1(WDC65816* cpu);
static void jml_indr_2(WDC65816* cpu);

static void schedule_jsr_a(WDC65816* cpu);
static void jsr_a_1(WDC65816* cpu);
static void jsr_a_2(WDC65816* cpu);
static void jsr_a_3(WDC65816* cpu);

static void schedule_jsr_a_x_indr(WDC65816* cpu);
static void jsr_a_x_indr_1(WDC65816* cpu);
static void jsr_a_x_indr_2(WDC65816* cpu);

static void schedule_jsl(WDC65816* cpu);
static void jsl_1(WDC65816* cpu);
static void jsl_2(WDC65816* cpu);

static void schedule_block_move(WDC65816* cpu);
static void block_move_1(WDC65816* cpu);
static void block_move_2(WDC65816* cpu);
static void block_move_3(WDC65816* cpu);
static void block_move_4(WDC65816* cpu);
static void block_move_5(WDC65816* cpu);

static void schedule_mvn(WDC65816* cpu);
static void mvn_1(WDC65816* cpu);

static void schedule_mvp(WDC65816* cpu);
static void mvp_1(WDC65816* cpu);

static void schedule_nop(WDC65816* cpu);
static void nop_1(WDC65816* cpu);

static void schedule_push_op_8(WDC65816* cpu);
static void push_op_8_1(WDC65816* cpu);

static void schedule_push_op_16(WDC65816* cpu);
static void push_op_16_1(WDC65816* cpu);
static void push_op_16_2(WDC65816* cpu);

static void schedule_pea(WDC65816* cpu);
static void pea_1(WDC65816* cpu);
static void pea_2(WDC65816* cpu);

static void schedule_pei(WDC65816* cpu);
static void pei_1(WDC65816* cpu);
static void pei_2(WDC65816* cpu);

static void schedule_per(WDC65816* cpu);
static void per_1(WDC65816* cpu);
static void per_2(WDC65816* cpu);
static void per_3(WDC65816* cpu);

static void schedule_phb(WDC65816* cpu);
static void schedule_phd(WDC65816* cpu);
static void schedule_phk(WDC65816* cpu);
static void schedule_php(WDC65816* cpu);

static void schedule_pha_8(WDC65816* cpu);
static void schedule_pha_16(WDC65816* cpu);

static void schedule_phx_8(WDC65816* cpu);
static void schedule_phx_16(WDC65816* cpu);

static void schedule_phy_8(WDC65816* cpu);
static void schedule_phy_16(WDC65816* cpu);

static void schedule_pla_8(WDC65816* cpu);
static void schedule_pla_16(WDC65816* cpu);
static void pla_8_1(WDC65816* cpu);
static void pla_16_1(WDC65816* cpu);
static void pla_16_2(WDC65816* cpu);

static void schedule_plb(WDC65816* cpu);
static void plb_1(WDC65816* cpu);

static void schedule_pld(WDC65816* cpu);
static void pld_1(WDC65816* cpu);
static void pld_2(WDC65816* cpu);

static void schedule_plp(WDC65816* cpu);
static void plp_1(WDC65816* cpu);

static void schedule_plx_8(WDC65816* cpu);
static void schedule_plx_16(WDC65816* cpu);
static void plx_8_1(WDC65816* cpu);
static void plx_16_1(WDC65816* cpu);
static void plx_16_2(WDC65816* cpu);

static void schedule_ply_8(WDC65816* cpu);
static void schedule_ply_16(WDC65816* cpu);
static void ply_8_1(WDC65816* cpu);
static void ply_16_1(WDC65816* cpu);
static void ply_16_2(WDC65816* cpu);

static void schedule_rep(WDC65816* cpu);
static void rep_1(WDC65816* cpu);
static void rep_2(WDC65816* cpu);

static void schedule_rts(WDC65816* cpu);
static void rts_1(WDC65816* cpu);
static void rts_2(WDC65816* cpu);
static void rts_3(WDC65816* cpu);

static void schedule_rtl(WDC65816* cpu);
static void rtl_1(WDC65816* cpu);

static void schedule_rti(WDC65816* cpu);
static void rti_1(WDC65816* cpu);
static void rti_2(WDC65816* cpu);
static void rti_3(WDC65816* cpu);
static void rti_4(WDC65816* cpu);

static void schedule_sec(WDC65816* cpu);
static void sec_1(WDC65816* cpu);

static void schedule_sed(WDC65816* cpu);
static void sed_1(WDC65816* cpu);

static void schedule_sei(WDC65816* cpu);
static void sei_1(WDC65816* cpu);

static void schedule_sep(WDC65816* cpu);
static void sep_1(WDC65816* cpu);
static void sep_2(WDC65816* cpu);

static void schedule_store_op_8(WDC65816* cpu);
static void schedule_store_op_16(WDC65816* cpu);
static void store_op_8_1(WDC65816* cpu);
static void store_op_16_1(WDC65816* cpu);
static void store_op_16_2(WDC65816* cpu);

static void schedule_sta_8(WDC65816* cpu);
static void schedule_sta_16(WDC65816* cpu);

static void schedule_stz_8(WDC65816* cpu);
static void schedule_stz_16(WDC65816* cpu);

static void schedule_stx_8(WDC65816* cpu);
static void schedule_stx_16(WDC65816* cpu);

static void schedule_sty_8(WDC65816* cpu);
static void schedule_sty_16(WDC65816* cpu);

static void schedule_tax_8(WDC65816* cpu);
static void schedule_tax_16(WDC65816* cpu);
static void tax_8_1(WDC65816* cpu);
static void tax_16_1(WDC65816* cpu);

static void schedule_tay_8(WDC65816* cpu);
static void schedule_tay_16(WDC65816* cpu);
static void tay_8_1(WDC65816* cpu);
static void tay_16_1(WDC65816* cpu);

static void schedule_tcd(WDC65816* cpu);
static void tcd_1(WDC65816* cpu);

static void schedule_tcs(WDC65816* cpu);
static void tcs_1(WDC65816* cpu);

static void schedule_tdc(WDC65816* cpu);
static void tdc_1(WDC65816* cpu);

static void schedule_tsc(WDC65816* cpu);
static void tsc_1(WDC65816* cpu);

static void schedule_tsx_8(WDC65816* cpu);
static void schedule_tsx_16(WDC65816* cpu);
static void tsx_8_1(WDC65816* cpu);
static void tsx_16_1(WDC65816* cpu);

static void schedule_txa_8(WDC65816* cpu);
static void schedule_txa_16(WDC65816* cpu);
static void txa_8_1(WDC65816* cpu);
static void txa_16_1(WDC65816* cpu);

static void schedule_txs(WDC65816* cpu);
static void txs_1(WDC65816* cpu);

static void schedule_txy_8(WDC65816* cpu);
static void schedule_txy_16(WDC65816* cpu);
static void txy_8_1(WDC65816* cpu);
static void txy_16_1(WDC65816* cpu);

static void schedule_tya_8(WDC65816* cpu);
static void schedule_tya_16(WDC65816* cpu);
static void tya_8_1(WDC65816* cpu);
static void tya_16_1(WDC65816* cpu);

static void schedule_tyx_8(WDC65816* cpu);
static void schedule_tyx_16(WDC65816* cpu);
static void tyx_8_1(WDC65816* cpu);
static void tyx_16_1(WDC65816* cpu);

static void schedule_wai(WDC65816* cpu);
static void wai_1(WDC65816* cpu);

static void schedule_wdm(WDC65816* cpu);
static void wdm_1(WDC65816* cpu);

static void schedule_xba(WDC65816* cpu);
static void xba_1(WDC65816* cpu);

static void schedule_xce(WDC65816* cpu);
static void xce_1(WDC65816* cpu);

static void reset_1(WDC65816* cpu);
static void reset_2(WDC65816* cpu);
static void reset_3(WDC65816* cpu);