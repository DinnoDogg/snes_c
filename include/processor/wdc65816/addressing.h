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

static void schedule_addr_d(WDC65816* cpu);
static void addr_d_1(WDC65816* cpu); 
static void addr_d_2(WDC65816* cpu); 

static void schedule_addr_d_x(WDC65816* cpu);
static void schedule_addr_d_y(WDC65816* cpu);

static void schedule_addr_d_ind(WDC65816* cpu);
static void addr_d_ind_1(WDC65816* cpu); 

static void schedule_addr_d_indr(WDC65816* cpu);
static void addr_d_indr_1(WDC65816* cpu); 
static void addr_d_indr_2(WDC65816* cpu); 
static void addr_d_indr_3(WDC65816* cpu); 
static void addr_d_indr_4(WDC65816* cpu); 

static void schedule_addr_d_indr_y(WDC65816* cpu);
static void addr_d_indr_y_1(WDC65816* cpu); 
static void addr_d_indr_y_2(WDC65816* cpu); 

static void schedule_addr_dl_indr(WDC65816* cpu);
static void addr_dl_indr_1(WDC65816* cpu); 
static void addr_dl_indr_2(WDC65816* cpu); 
static void addr_dl_indr_3(WDC65816* cpu); 

static void schedule_addr_dl_indr_y(WDC65816* cpu);
static void addr_dl_indr_y_1(WDC65816* cpu); 

static void schedule_addr_d_x_indr(WDC65816* cpu);
static void addr_d_x_indr_1(WDC65816* cpu); 

static void schedule_addr_d_s(WDC65816* cpu);
static void addr_d_s_1(WDC65816* cpu); 
static void addr_d_s_2(WDC65816* cpu); 

static void schedule_addr_d_s_indr_y(WDC65816* cpu);
static void addr_d_s_indr_y_1(WDC65816* cpu);
static void addr_d_s_indr_y_2(WDC65816* cpu);
static void addr_d_s_indr_y_3(WDC65816* cpu);
static void addr_d_s_indr_y_4(WDC65816* cpu);
static void addr_d_s_indr_y_5(WDC65816* cpu);

static void schedule_addr_imm(WDC65816* cpu);
static void schedule_addr_imm_x(WDC65816* cpu);

static void schedule_addr_implied(WDC65816* cpu);