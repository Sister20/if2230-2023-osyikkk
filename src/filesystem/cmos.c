#include "../lib-header/cmos.h"

static struct dateNtime currentdatentime;

void init_cmos(){
    read_cmos();
}

int8_t is_update_cmos() {
      out(CMOS_ADDRESS, 0x0A);
      return (in(CMOS_DATA) & 0x80);
}
 
uint8_t get_cmos_reg(int reg) {
      out(CMOS_ADDRESS, reg);
      return in(CMOS_DATA);
}

void set_cmos_reg(int reg, uint8_t value){
    out(CMOS_ADDRESS, reg);
    out(CMOS_DATA, value);
}

void read_cmos(){
    while(is_update_cmos());

    currentdatentime.time.second = get_cmos_reg(0x00);
    currentdatentime.time.minute = get_cmos_reg(0x02);
    currentdatentime.time.hour = get_cmos_reg(0x04);
    currentdatentime.date.day = get_cmos_reg(0x07);
    currentdatentime.date.month = get_cmos_reg(0x08);
    currentdatentime.date.year = get_cmos_reg(0x09);

    uint8_t regB = get_cmos_reg(0x0B);

    if(!(regB & 0x04)){
        currentdatentime.time.second = (currentdatentime.time.second & 0x0F) + ((currentdatentime.time.second / 16) * 10);
        currentdatentime.time.minute = (currentdatentime.time.minute & 0x0F) + ((currentdatentime.time.minute / 16) * 10);
        currentdatentime.time.hour = ((currentdatentime.time.hour & 0x0F) + (((currentdatentime.time.hour & 0x70) / 16) * 10)) | (currentdatentime.time.hour & 0x80);
        currentdatentime.date.day = (currentdatentime.date.day & 0x0F) + ((currentdatentime.date.day / 16) * 10);
        currentdatentime.date.month = (currentdatentime.date.month & 0x0F) + ((currentdatentime.date.month / 16) * 10);
        currentdatentime.date.year = (currentdatentime.date.year & 0x0F) + ((currentdatentime.date.year / 16) * 10);
    }
}

void write_cmos(struct dateNtime * dnt){
    while (is_update_cmos());

    set_cmos_reg(0x00, dnt->time.second);
    set_cmos_reg(0x02, dnt->time.minute);
    set_cmos_reg(0x04, dnt->time.hour);
    set_cmos_reg(0x07, dnt->date.day);
    set_cmos_reg(0x08, dnt->date.month);
    set_cmos_reg(0x09, dnt->date.year);
}

int8_t is_leap_year(uint16_t year){
    if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) return 1;
    return 0;
}

struct date get_currdate(){
    return currentdatentime.date;
}

struct time get_currtime(){
    return currentdatentime.time;
}