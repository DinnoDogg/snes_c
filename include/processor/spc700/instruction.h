static void mov_a_mem(SPC700* cpu);
static void mov_x_mem(SPC700* cpu);
static void mov_y_mem(SPC700* cpu);

static void mov_mem_a(SPC700* cpu);
static void mov_mem_x(SPC700* cpu);
static void mov_mem_y(SPC700* cpu);

static void mov_a_x(SPC700* cpu);
static void mov_a_y(SPC700* cpu);
static void mov_x_a(SPC700* cpu);
static void mov_y_a(SPC700* cpu);
static void mov_x_sp(SPC700* cpu);
static void mov_sp_x(SPC700* cpu);
static void mov_dp_dp(SPC700* cpu);
static void mov_dp_imm(SPC700* cpu);

static void adc_a(SPC700* cpu);
static void adc_indr_indr(SPC700* cpu);
static void adc_dp_dp(SPC700* cpu);
static void adc_dp_imm(SPC700* cpu);

static void sbc_a(SPC700* cpu);
static void sbc_indr_indr(SPC700* cpu);
static void sbc_dp_dp(SPC700* cpu);
static void sbc_dp_imm(SPC700* cpu);

static void cmp_a(SPC700* cpu);
static void cmp_indr_indr(SPC700* cpu);
static void cmp_dp_dp(SPC700* cpu);
static void cmp_dp_imm(SPC700* cpu);

static void cmp_x(SPC700* cpu);
static void cmp_y(SPC700* cpu);

static void and_a(SPC700* cpu);
static void and_indr_indr(SPC700* cpu);
static void and_dp_dp(SPC700* cpu);
static void and_dp_imm(SPC700* cpu);

static void or_a(SPC700* cpu);
static void or_indr_indr(SPC700* cpu);
static void or_dp_dp(SPC700* cpu);
static void or_dp_imm(SPC700* cpu);

static void eor_a(SPC700* cpu);
static void eor_indr_indr(SPC700* cpu);
static void eor_dp_dp(SPC700* cpu);
static void eor_dp_imm(SPC700* cpu);

static void inc_a(SPC700* cpu);
static void inc_mem(SPC700* cpu);

static void dec_a(SPC700* cpu);
static void dec_mem(SPC700* cpu);

static void inc_x(SPC700* cpu);
static void inc_y(SPC700* cpu);

static void dec_x(SPC700* cpu);
static void dec_y(SPC700* cpu);

static void asl_a(SPC700* cpu);
static void asl_mem(SPC700* cpu);

static void lsr_a(SPC700* cpu);
static void lsr_mem(SPC700* cpu);

static void rol_a(SPC700* cpu);
static void rol_mem(SPC700* cpu);

static void ror_a(SPC700* cpu);
static void ror_mem(SPC700* cpu);

static void xcn_a(SPC700* cpu);

static void movw_ya_dp(SPC700* cpu);
static void movw_dp_ya(SPC700* cpu);

static void incw_dp(SPC700* cpu);
static void decw_dp(SPC700* cpu);

static void addw_dp(SPC700* cpu);
static void subw_dp(SPC700* cpu);
static void cmpw_dp(SPC700* cpu);

static void mul_ya(SPC700* cpu);
static void div_ya(SPC700* cpu);

static void daa(SPC700* cpu);
static void das(SPC700* cpu);

static void bra(SPC700* cpu);
static void beq(SPC700* cpu);
static void bne(SPC700* cpu);
static void bcs(SPC700* cpu);
static void bcc(SPC700* cpu);
static void bvs(SPC700* cpu);
static void bvc(SPC700* cpu);
static void bmi(SPC700* cpu);
static void bpl(SPC700* cpu);

static void bbs(SPC700* cpu);
static void bbc(SPC700* cpu);

static void cbne(SPC700* cpu);

static void dbnz_mem(SPC700* cpu);
static void dbnz_y(SPC700* cpu);

static void jmp(SPC700* cpu);

static void call(SPC700* cpu);
static void pcall(SPC700* cpu);
static void tcall(SPC700* cpu);

static void brk(SPC700* cpu);

static void ret(SPC700* cpu);
static void reti(SPC700* cpu);

static void push_a(SPC700* cpu);
static void push_x(SPC700* cpu);
static void push_y(SPC700* cpu);
static void push_psw(SPC700* cpu);

static void pop_a(SPC700* cpu);
static void pop_x(SPC700* cpu);
static void pop_y(SPC700* cpu);
static void pop_psw(SPC700* cpu);

static void set_mem_bit(SPC700* cpu);
static void clr_mem_bit(SPC700* cpu);

static void tset1(SPC700* cpu);
static void tclr1(SPC700* cpu);

static void and1(SPC700* cpu);
static void nand1(SPC700* cpu);

static void or1(SPC700* cpu);
static void nor1(SPC700* cpu);

static void eor1(SPC700* cpu);

static void not1(SPC700* cpu);

static void mov1_c_mem(SPC700* cpu);
static void mov1_mem_c(SPC700* cpu);

static void clcr(SPC700* cpu);
static void setc(SPC700* cpu);
static void notc(SPC700* cpu);

static void clrv(SPC700* cpu);

static void clrp(SPC700* cpu);
static void sep(SPC700* cpu);

static void ei(SPC700* cpu);
static void di(SPC700* cpu);

static void nop(SPC700* cpu);