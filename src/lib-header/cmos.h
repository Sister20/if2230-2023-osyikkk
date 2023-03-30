#ifndef _CMOS_H
#define _CMOS_H

// TODO : Belum diintegrasiin ke fat32

#include "portio.h"
#include "interrupt.h"

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
    struct time time;
    struct date date;
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

int8_t is_update_cmos();

uint8_t get_cmos_reg(int reg);

void set_cmos_reg(int reg, uint8_t value);
      
void read_cmos();

void write_cmos(struct dateNtime * dnt);

int8_t is_leap_year(uint16_t year);

struct date get_currdate();

struct time get_currtime();

#endif