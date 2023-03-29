#include "../lib-header/cmos.h"

int8_t get_update_in_progress_flag() {
      out(CMOS_ADDRESS, 0x0A);
      return (in(CMOS_DATA) & 0x80);
}
 
uint8_t get_RTC_register(int reg) {
      out(CMOS_ADDRESS, reg);
      return in(CMOS_DATA);
}

void read_cmos(){
    while (!get_update_in_progress_flag()){
        // second = get_RTC_register(CMOS_SECONDS);
        // minute = get_RTC_register(CMOS_MINUTES);
        // hour = get_RTC_register(CMOS_HOURS);
        // day = get_RTC_register(CMOS_DAY_OF_MONTH);
        // month = get_RTC_register(CMOS_MONTH);
        // year = get_RTC_register(CMOS_YEAR);
    }
}