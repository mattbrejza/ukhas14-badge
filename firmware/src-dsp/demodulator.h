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

typedef struct s_bit_sync_state {
	uint8_t pos;
	int8_t late;
	int8_t early;
	int8_t error;
	int8_t error_i;
	uint8_t vco;	//full bit period = 16*16 = 256
} t_bit_sync_state;

typedef struct s_char_count_state {
	uint8_t bit_counter;
	char current_char;
	uint8_t mask;
	uint8_t sample_counter;
} t_char_count_state;


typedef struct s_char_sync_state{
	uint8_t bit_counter;
	char current_char;
	uint8_t mask;
} t_char_sync_state;



void fir_filter(t_fir_state *state, int32_t *input, int32_t *output, uint16_t len);
uint16_t cic_filter(t_cic_state *state, int32_t *input, int32_t *output, uint16_t len);
uint16_t bit_sync(t_bit_sync_state *state, int32_t *input, int32_t *output, uint16_t len);
uint16_t char_sync(t_char_sync_state *state, int32_t *input, char *output, uint16_t len, uint8_t data_bits);
uint16_t char_sync_count(t_char_count_state *state, int32_t *input, char *output, uint16_t len, uint8_t data_bits);

