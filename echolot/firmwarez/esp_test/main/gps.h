#pragma once

typedef struct
{
    uint8_t valid;

    uint8_t hour;
    uint8_t min;
    uint8_t sec;

    char time[12];

    uint8_t day;
    uint8_t month;
    uint8_t year;

    char date[12];

    double lat;
    double lon;

	uint8_t satnum;
	uint8_t coord_found;

} gps_data_t;

void gps_init(void);

void gps_get_data();


void gps_get_info_text(char *s);

int parse_gprmc(const char *s, gps_data_t *gps);

int parse_gps(const char *s, gps_data_t *gps);

void gps_info(gps_data_t *gps, char *str);

void print_gps(gps_data_t *gps);
