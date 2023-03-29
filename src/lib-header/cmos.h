#ifndef _CMOS_H
#define _CMOS_H

// TODO : Implement the CMOS driver

#include "portio.h"
// CMOS
#define CURRENT_YEAR 2023                                    
#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

#define REG_SECONDS 0x00
#define REG_MINUTES 0x02
#define REG_HOURS 0x04
#define REG_DAY_OF_MONTH 0x07
#define REG_MONTH 0x08
#define REG_YEAR 0x09   

typedef struct dateNtime{
    struct date date;
    struct time time;
} __attribute__((packed));

typedef struct date
{
    uint8_t day;
    uint8_t month;
    uint16_t year;
} __attribute__((packed));

typedef struct time
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
} __attribute__((packed));

void init_cmos();

int8_t get_update_in_progress_flag();

uint8_t get_cmos_reg(int reg);
      
void read_cmos();

int8_t is_leap_year(uint8_t year, uint8_t month);

struct date get_date(struct dateNtime dateNtime);

struct time get_time(struct dateNtime dateNtime);

#endif