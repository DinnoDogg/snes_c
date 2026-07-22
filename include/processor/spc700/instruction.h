static void schedule_mov_a_mem(SPC700* cpu);
static void mov_a_mem_1(SPC700* cpu);

static void schedule_mov_x_mem(SPC700* cpu);
static void mov_x_mem_1(SPC700* cpu);

static void schedule_mov_y_mem(SPC700* cpu);
static void mov_y_mem_1(SPC700* cpu);

static void schedule_mov_mem_a(SPC700* cpu);
static void mov_mem_a_1(SPC700* cpu);

static void schedule_mov_mem_x(SPC700* cpu);
static void mov_mem_x_1(SPC700* cpu);

static void schedule_mov_mem_y(SPC700* cpu);
static void mov_mem_y_1(SPC700* cpu);

static void schedule_txa(SPC700* cpu);
static void txa_1(SPC700* cpu);

static void schedule_tya(SPC700* cpu);
static void tya_1(SPC700* cpu);

static void schedule_tax(SPC700* cpu);
static void tax_1(SPC700* cpu);

static void schedule_tay(SPC700* cpu);
static void tay_1(SPC700* cpu);

static void schedule_tsx(SPC700* cpu);
static void tsx_1(SPC700* cpu);

static void schedule_txs(SPC700* cpu);
static void txs_1(SPC700* cpu);

static void schedule_mov_dp_dp(SPC700* cpu);
static void mov_dp_dp_1(SPC700* cpu);
static void mov_dp_dp_2(SPC700* cpu);

static void schedule_mov_dp_imm(SPC700* cpu);
static void mov_dp_imm_1(SPC700* cpu);