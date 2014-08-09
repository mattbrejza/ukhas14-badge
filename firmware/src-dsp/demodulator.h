typedef struct s_fir_state {
	const int32_t *coeff;
	int32_t *delay_line;
    uint8_t length;
    uint8_t delay_ptr;
} t_fir_state;

typedef struct s_cic_state {
	int32_t *delay_line_in;
	int32_t *delay_line_out;
    uint8_t stages;
    uint8_t rate;
    uint8_t rate_count;
} t_cic_state;


void fir_filter(t_fir_state state, int32_t *input, int32_t *output, uint16_t len);
uint16_t cic_filter(t_cic_state state, int32_t *input, int32_t *output, uint16_t len);
