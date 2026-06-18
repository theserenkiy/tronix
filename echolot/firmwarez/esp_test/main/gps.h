#pragma once

typedef struct
{
    uint8_t valid;

    uint8_t hour;
    uint8_t min;
    uint8_t sec;

    char time[12];
    uint8_t time_ok;

    uint8_t day;
    uint8_t month;
    uint8_t year;

    char date[12];
    uint8_t date_ok;

    double lat;
    double lon;

	uint8_t satnum;
	uint8_t coord_found;

} gps_data_t;

void gps_init(void);

void gps_read();

void gps_get_data(gps_data_t *gps);

int parse_gprmc(const char *s, gps_data_t *gps);

int parse_gps(const char *s, gps_data_t *gps);

void gps_info(char *str);

void print_gps(gps_data_t *gps);

void setTime(uint16_t H, uint16_t M, uint16_t S, uint16_t Y, uint16_t m, uint16_t d);
