static void schedule_addr_a(WDC65816* cpu);
static void addr_a_1(WDC65816* cpu);
static void addr_a_2(WDC65816* cpu);

static void schedule_addr_a_x(WDC65816* cpu);
static void schedule_addr_a_y(WDC65816* cpu);

static void schedule_addr_a_ind(WDC65816* cpu);
static void addr_a_ind_1(WDC65816* cpu); 
static void addr_a_ind_2(WDC65816* cpu); 

static void schedule_addr_al(WDC65816* cpu);
static void addr_al_1(WDC65816* cpu); 
static void addr_al_2(WDC65816* cpu); 

static void schedule_addr_al_x(WDC65816* cpu);
static void addr_al_x_1(WDC65816* cpu); 

static void schedule_addr_d_x_indr(WDC65816* cpu);