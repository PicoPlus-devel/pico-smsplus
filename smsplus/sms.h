
#ifndef _SMS_H_
#define _SMS_H_

#define TYPE_OVERSEAS   (0)
#define TYPE_DOMESTIC   (1)

#define SMS_WIDTH 256
#define SMS_HEIGHT 192
#define SMS_AUD_RATE 44100
#define SMS_FPS 60
// rendering
static uint8_t screenCropX = 0;
static uint16_t screenBufferLine[256];
static uint8_t smsBufferLine[SMS_WIDTH];

// FH - 2019-06-30: Added support for RGB444 in stead of RGB565
static int palette444[32];

#define SRAMSIZE 0x8000
#define SRAMSIZEBYTES ( 0x8000 * sizeof(uint8) )
/* SG-1000 has no battery RAM; sms.sram is an 8 KB cartridge RAM expansion there */
#define SG_EXTRAMSIZE 0x2000
#define SRAM_BYTES ( IS_SG ? (SG_EXTRAMSIZE * sizeof(uint8)) : SRAMSIZEBYTES )
#define RAMSIZE  0x2000
#define RAMSIZEBYTES  (0x2000 * sizeof(uint8))

//static uint8 sram[SRAMSIZE];
/* SMS context */
typedef struct {
    uint8 *dummy; //JMD: Point this into outher space plz.
    uint8 *ram;   // [RAMSIZE];
//    uint8 sram[0x8000];
    uint8 *sram;
    uint8 fcr[4];
    uint8 paused;
    uint8 save;
    uint8 country;
    uint8 port_3F;      /* SG-1000 (no port $3F): bit n set = RAM adaptor on page n */
    uint8 port_F2;
    uint8 use_fm;
    uint8 irq;
    uint8 psg_mask;
} t_sms;

/* Global data */
extern t_sms sms;

/* Function prototypes */
void sms_frame(int skip_render);

void sms_init(void);

void sms_reset(void);

int sms_irq_callback(int param);

void sms_mapper_w(int address, int data);

void sg_memory_map(void);

void cpu_reset(void);

#endif /* _SMS_H_ */
