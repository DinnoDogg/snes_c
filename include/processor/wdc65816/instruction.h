static void schedule_rmw(WDC65816* cpu);
static void rmw_8_1(WDC65816* cpu);
static void rmw_8_2(WDC65816* cpu);
static void rmw_8_3(WDC65816* cpu);

static void rmw_16_1(WDC65816* cpu);
static void rmw_16_2(WDC65816* cpu);
static void rmw_16_3(WDC65816* cpu);
static void rmw_16_4(WDC65816* cpu);
static void rmw_16_5(WDC65816* cpu);

static void schedule_alu_implicit(WDC65816* cpu);
static void alu_8_1(WDC65816* cpu);
static void alu_16_1(WDC65816* cpu);
static void alu_16_2(WDC65816* cpu);

static void schedule_alu_memory(WDC65816* cpu);
static void alu_memory_8_1(WDC65816* cpu);
static void alu_memory_16_1(WDC65816* cpu);
static void alu_memory_16_2(WDC65816* cpu);

static void schedule_alu_memory_writeback(WDC65816* cpu);
static void alu_memory_writeback_8_1(WDC65816* cpu);
static void alu_memory_writeback_16_1(WDC65816* cpu);

static void schedule_alu(WDC65816* cpu);
static void alu_8_1(WDC65816* cpu);
static void alu_16_1(WDC65816* cpu);

static void schedule_and(WDC65816* cpu);
static void schedule_eor(WDC65816* cpu);
static void schedule_bit(WDC65816* cpu);
static void schedule_cmp(WDC65816* cpu);
static void schedule_lda(WDC65816* cpu);
static void schedule_ora(WDC65816* cpu);

static void schedule_asl(WDC65816* cpu);
static void schedule_asl_a(WDC65816* cpu);

static void schedule_dec(WDC65816* cpu);
static void schedule_dec_a(WDC65816* cpu);

static void schedule_lsr(WDC65816* cpu);
static void schedule_lsr_a(WDC65816* cpu);

static void schedule_rol(WDC65816* cpu);
static void schedule_rol_a(WDC65816* cpu);

static void schedule_ror(WDC65816* cpu);
static void schedule_ror_a(WDC65816* cpu);

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

static void schedule_flag_clear(WDC65816* cpu);
static void flag_clear_1(WDC65816* cpu);

static void schedule_flag_set(WDC65816* cpu);
static void flag_set_1(WDC65816* cpu);

static void schedule_clc(WDC65816* cpu);
static void schedule_clv(WDC65816* cpu);
static void schedule_cli(WDC65816* cpu);
static void schedule_cld(WDC65816* cpu);

static void schedule_cpx(WDC65816* cpu);
static void schedule_cpy(WDC65816* cpu);

static void schedule_dex(WDC65816* cpu);
static void schedule_dey(WDC65816* cpu);

static void schedule_inc(WDC65816* cpu);
static void schedule_inc_a(WDC65816* cpu);

static void schedule_inx(WDC65816* cpu);
static void schedule_iny(WDC65816* cpu);

static void schedule_ldx(WDC65816* cpu);
static void schedule_ldy(WDC65816* cpu);

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

static void schedule_pha(WDC65816* cpu);
static void schedule_phb(WDC65816* cpu);
static void schedule_phd(WDC65816* cpu);
static void schedule_phk(WDC65816* cpu);
static void schedule_php(WDC65816* cpu);

static void schedule_push_index(WDC65816* cpu);

static void schedule_phx(WDC65816* cpu);
static void schedule_phy(WDC65816* cpu);

static void schedule_pla(WDC65816* cpu);
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

static void schedule_pull_index(WDC65816* cpu);
static void pull_index_8_1(WDC65816* cpu);
static void pull_index_16_1(WDC65816* cpu);
static void pull_index_16_2(WDC65816* cpu);

static void schedule_plx(WDC65816* cpu);
static void schedule_ply(WDC65816* cpu);

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
static void schedule_sed(WDC65816* cpu);
static void schedule_sei(WDC65816* cpu);

static void schedule_sep(WDC65816* cpu);
static void sep_1(WDC65816* cpu);
static void sep_2(WDC65816* cpu);

static void schedule_store(WDC65816* cpu);
static void store_8_1(WDC65816* cpu);
static void store_16_1(WDC65816* cpu);
static void store_16_2(WDC65816* cpu);

static void schedule_sta(WDC65816* cpu);
static void schedule_stz(WDC65816* cpu);

static void schedule_store_indirect(WDC65816* cpu);

static void schedule_stx(WDC65816* cpu);
static void schedule_sty(WDC65816* cpu);

static void schedule_ta_index(WDC65816* cpu);
static void ta_index_8_1(WDC65816* cpu);
static void ta_index_16_1(WDC65816* cpu);

static void schedule_tay(WDC65816* cpu);
static void schedule_tax(WDC65816* cpu);

static void schedule_tcd(WDC65816* cpu);
static void tcd_1(WDC65816* cpu);

static void schedule_tcs(WDC65816* cpu);
static void tcs_1(WDC65816* cpu);

static void schedule_tdc(WDC65816* cpu);
static void tdc_1(WDC65816* cpu);

