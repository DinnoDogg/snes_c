static void schedule_alu(WDC65816* cpu);
static void alu_8_1(WDC65816* cpu);

static void alu_16_1(WDC65816* cpu);
static void alu_16_2(WDC65816* cpu);

static void schedule_rmw(WDC65816* cpu);
static void rmw_8_1(WDC65816* cpu);
static void rmw_8_2(WDC65816* cpu);
static void rmw_8_3(WDC65816* cpu);

static void rmw_16_1(WDC65816* cpu);
static void rmw_16_2(WDC65816* cpu);
static void rmw_16_3(WDC65816* cpu);
static void rmw_16_4(WDC65816* cpu);
static void rmw_16_5(WDC65816* cpu);

static void schedule_accumulator(WDC65816* cpu);
static void accumulator_8_1(WDC65816* cpu);
static void accumulator_16_1(WDC65816* cpu);

static void schedule_and(WDC65816* cpu);

static void schedule_asl(WDC65816* cpu);
static void schedule_asl_a(WDC65816* cpu);