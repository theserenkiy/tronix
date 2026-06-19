#include <stdio.h>
#include "driver/uart.h"
#include "common.h"
#include "gps.h"
#include <time.h>
#include <sys/time.h>

#define GPS_UART UART_NUM_2

gps_data_t gps_data = {
	.time_ok = 0,
	.date_ok = 0,
	.coord_found = 0
};

void gps_init(void)
{
	printf("Initing GPS...\n");

	DSTAT->gps = &gps_data;

	gpio_reset_pin(GPS_ENA_PIN);
	gpio_set_direction(GPS_ENA_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(GPS_ENA_PIN, 0);

	gps_enable(1);

    uart_config_t cfg = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(GPS_UART, 4096, 0, 0, NULL, 0);
    uart_param_config(GPS_UART, &cfg);

    uart_set_pin(
        GPS_UART,
        GPS_RX_PIN,
        GPS_TX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE
    );



	printf("GPS init done!\n");
}

void gps_task(void *prm)
{
	while(1)
	{
		vTaskDelay(pdMS_TO_TICKS(3000));
		if(!DSTAT->gps_enabled)
			continue;

		gps_read();
	}
}

void gps_enable(uint8_t state)
{
	printf("GPS ENABLE: %d\n",state);
	gpio_set_level(GPS_ENA_PIN, state);
	DSTAT->gps_enabled = state;
}

void gps_read()
{
	gps_get_data(&gps_data);
}

void gps_get_data(gps_data_t *gps)
{
    char str[512];

	int len = uart_read_bytes(
		GPS_UART,
		str,
		sizeof(str)-1,
		pdMS_TO_TICKS(1000));

	if (len > 0)
	{
		str[len] = 0;
		// printf("\n=============\n");
		// printf("%s", buf);

		// if()
		// {
		// 	return 1;
		// }
		// else
		// {
		// 	printf("No valid GPS data\n");
		// 	return 0;
		// }

		parse_gps(str, gps);
		if(gps->time_ok && !DSTAT->time_set)
		{
			setTime(gps->hour,gps->min,gps->sec,0,0,0);
			DSTAT->time_set = 1;
		}

		if(gps->date_ok && !DSTAT->date_set)
		{
			setTime(gps->hour,gps->min,gps->sec,gps->year,gps->month,gps->day);
			DSTAT->date_set = 1;
		}

		if(gps->coord_found)
		{
			
			time(&DSTAT->gps_updtime);
			DSTAT->lon = gps->lon;
			DSTAT->lat = gps->lat;
			DSTAT->satnum = gps->satnum;
			sprintf(DSTAT->gps_str, "%.4f  %.4f S%d", gps->lat, gps->lon, gps->satnum);
		}

	}
	else
	{
		printf("GPS MODULE RETURNED NO DATA\n");
		return;
	}
}


static double nmea_to_deg(double nmea)
{
    int deg = (int)(nmea / 100);
    double min = nmea - deg * 100;

    return deg + min / 60.0;
}

int parse_gprmc(const char *s, gps_data_t *gps)
{
    char status;
    char ns;
    char ew;

    double lat_raw;
    double lon_raw;

    float utc;
    int date;

    char *substr = "$GPRMC";
    char *ptr = strstr(s, substr);

    if(!ptr)
    {
        printf("%s not found\n",substr);
        return 0;
    }

	int n = sscanf(
		ptr,
		"$GPRMC,%f",
		&utc
	);

	if(!n)
	{
		printf("No data found for %s\n",substr);
		return 0;
	}

	gps->hour = ((int)utc / 10000);
    gps->min  = ((int)utc / 100) % 100;
    gps->sec  = ((int)utc) % 100;
	gps->time_ok = 1;

	
	sprintf(gps->time,"%02d:%02d:%02d",gps->hour, gps->min, gps->sec);

	gps->coord_found = 0;

	gps->time[8] = 0;

    n = sscanf(
        ptr,
        "$GPRMC,%*f,%c,%lf,%c,%lf,%c,%*f,%*f,%d",
        &status,
        &lat_raw,
        &ns,
        &lon_raw,
        &ew,
        &date);

    if (n != 6)
    {
        printf("Incorrect data count for %s: %d\n",substr,n);
        return 1;
    }

	gps->coord_found = 1;
    gps->valid = (status == 'A');

    sprintf(gps->time,"%02d:%02d:%02d",gps->hour, gps->min, gps->sec);
	gps->time[8] = 0;

    gps->day   = date / 10000;
    gps->month = (date / 100) % 100;
    gps->year  = date % 100;
	gps->date_ok = 1;

    sprintf(gps->date,"%02d.%02d.%02d",gps->day, gps->month, gps->year);
	gps->date[8] = 0;

    gps->lat = nmea_to_deg(lat_raw);
    gps->lon = nmea_to_deg(lon_raw);

    if (ns == 'S')
        gps->lat = -gps->lat;

    if (ew == 'W')
        gps->lon = -gps->lon;

    return 1;
}

int parse_gpgga(const char *s, gps_data_t *gps)
{
	int satnum;
	char *substr = "$GPGGA";
    char *ptr = strstr(s, substr);

    if(!ptr)
    {
        printf("%s not found\n",substr);
        return 0;
    }

    int n = sscanf(
        ptr,
        "$GPGGA,%*f,%*f,%*c,%*f,%*c,%*d,%d",
        &satnum
	);

    if (n != 1)
    {
        printf("Incorrect data count for %s: %d\n",substr, n);
        return 0;
    }

	gps->satnum = satnum;

	return 1;
	
}

int parse_gps(const char *s, gps_data_t *gps)
{
	if(!parse_gprmc(s, gps))
		return 0;

	if(!parse_gpgga(s, gps))
		return 0;
	
	return 1;
}



void setTime(uint16_t H, uint16_t M, uint16_t S, uint16_t Y, uint16_t m, uint16_t d)
{
	struct tm t;
	if(!Y)
	{
		Y = 19;
		m = 6;
		d = 16;
	}

	printf("SET TIME %d:%d:%d %d.%d.%d\n",H,M,S,Y,m,d);

	t.tm_year = 100+Y; // Год минус 1900
	t.tm_mon = m-1;            // Месяц (0 - Январь, 11 - Декабрь)
	t.tm_mday = d;           // День месяца (1-31)
	t.tm_hour = H;         // Часы (0-23)
	t.tm_min = M;            // Минуты (0-59)
	t.tm_sec = S;            // Секунды (0-59)

	// 2. Преобразуйте в формат time_t
	time_t time_to_set = mktime(&t)+3600*3;

	// 3. Примените системное время
	struct timeval tv = { .tv_sec = time_to_set, .tv_usec = 0 };
	settimeofday(&tv, NULL);
}