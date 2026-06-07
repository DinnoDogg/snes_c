static void schedule_rmw(WDC65816* cpu);
static void rmw_8_1(WDC65816* cpu);
static void rmw_8_2(WDC65816* cpu);
static void rmw_8_3(WDC65816* cpu);

static void rmw_16_1(WDC65816* cpu);
static void rmw_16_2(WDC65816* cpu);
static void rmw_16_3(WDC65816* cpu);
static void rmw_16_4(WDC65816* cpu);
static void rmw_16_5(WDC65816* cpu);









static void schedule_op_accumulator(WDC65816* cpu);
static void accumulator_8_1(WDC65816* cpu);
static void accumulator_16_1(WDC65816* cpu);

static void schedule_op_index_reg(WDC65816* cpu);
static void index_reg_8_1(WDC65816* cpu);
static void index_reg_16_1(WDC65816* cpu);

static void schedule_alu_writeback(WDC65816* cpu);
static void alu_writeback_8_1(WDC65816* cpu);
static void alu_writeback_16_1(WDC65816* cpu);

static void schedule_alu(WDC65816* cpu);
static void alu_8_1(WDC65816* cpu);
static void alu_16_1(WDC65816* cpu);
static void alu_16_2(WDC65816* cpu);

static void schedule_alu_index_reg(WDC65816* cpu);
static void alu_index_8_1(WDC65816* cpu);
static void alu_index_16_1(WDC65816* cpu);

static void schedule_alu_index_reg_writeback(WDC65816* cpu);
static void alu_index_reg_writeback_8_1(WDC65816* cpu);
static void alu_index_reg_writeback_16_1(WDC65816* cpu);

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

static void schedule_asl(WDC65816* cpu);
static void schedule_asl_a(WDC65816* cpu);
static void schedule_dec(WDC65816* cpu);
static void schedule_dec_a(WDC65816* cpu);

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
static void jsr_a_4(WDC65816* cpu);

static void schedule_jsr_a_x_indr(WDC65816* cpu);
static void jsr_a_x_indr_1(WDC65816* cpu);
static void jsr_a_x_indr_2(WDC65816* cpu);

static void schedule_jsl(WDC65816* cpu);
static void jsl_1(WDC65816* cpu);
static void jsl_2(WDC65816* cpu);
static void jsl_3(WDC65816* cpu);

