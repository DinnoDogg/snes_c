static void schedule_addr_imm(SPC700* cpu);

static void schedule_addr_indr(SPC700* cpu);
static void addr_indr_1(SPC700* cpu);

static void schedule_addr_indr_inc(SPC700* cpu);
static void addr_indr_inc_1(SPC700* cpu);

static void schedule_addr_dp(SPC700* cpu);
static void addr_dp_1(SPC700* cpu);

static void schedule_addr_dp_x(SPC700* cpu);
static void addr_dp_x_1(SPC700* cpu);
static void addr_dp_x_2(SPC700* cpu);

static void schedule_addr_dp_y(SPC700* cpu);
static void addr_dp_y_1(SPC700* cpu);
static void addr_dp_y_2(SPC700* cpu);

static void schedule_addr_a(SPC700* cpu);
static void addr_a_1(SPC700* cpu);
static void addr_a_2(SPC700* cpu);

static void schedule_addr_a_x(SPC700* cpu);
static void addr_a_x_1(SPC700* cpu);
static void addr_a_x_2(SPC700* cpu);

static void schedule_addr_a_y(SPC700* cpu);
static void addr_a_y_1(SPC700* cpu);
static void addr_a_y_2(SPC700* cpu);

static void schedule_addr_dp_x_indr(SPC700* cpu);
static void addr_dp_x_indr_1(SPC700* cpu);
static void addr_dp_x_indr_2(SPC700* cpu);
static void addr_dp_x_indr_3(SPC700* cpu);

static void schedule_addr_dp_indr_y(SPC700* cpu);
static void addr_dp_indr_y_1(SPC700* cpu);
static void addr_dp_indr_y_2(SPC700* cpu);
static void addr_dp_indr_y_3(SPC700* cpu);
static void addr_dp_indr_y_4(SPC700* cpu);