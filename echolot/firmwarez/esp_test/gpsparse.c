#include <stdio.h>
#include <string.h>
#include <inttypes.h>


typedef struct
{
	int valid;

	uint8_t hour;
	uint8_t min;
	uint8_t sec;

	char time[9];

	uint8_t day;
	uint8_t month;
	uint8_t year;

	char date[9];

	double lat;
	double lon;

	uint8_t satnum;

} gps_data_t;

static double nmea_to_deg(double nmea)
{
	int deg = (int)(nmea / 100);
	double min = nmea - deg * 100;

	return deg + min / 60.0;
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

	printf("Time found: %.2f\n",utc);

	gps->hour = ((int)utc / 10000);
    gps->min  = ((int)utc / 100) % 100;
    gps->sec  = ((int)utc) % 100;

	sprintf(gps->time,"%02d:%02d:%02d\0",gps->hour, gps->min, gps->sec);

    n = sscanf(
        ptr,
        "$GPRMC,%f,%c,%lf,%c,%lf,%c,%*f,%*f,%d",
        &utc,
        &status,
        &lat_raw,
        &ns,
        &lon_raw,
        &ew,
        &date);

    if (n != 7)
    {
        printf("Incorrect data count for %s: %d\n",substr,n);
        return 1;
    }

    gps->valid = (status == 'A');

	

	gps->day   = date / 10000;
	gps->month = (date / 100) % 100;
	gps->year  = date % 100;

	sprintf(gps->date,"%02d.%02d.%02d\0",gps->day, gps->month, gps->year);

	gps->lat = nmea_to_deg(lat_raw);
	gps->lon = nmea_to_deg(lon_raw);

	if (ns == 'S')
		gps->lat = -gps->lat;

	if (ew == 'W')
		gps->lon = -gps->lon;

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

void print_gps(gps_data_t *gps)
{
	printf("Date: %s; Time: %s; LAT: %.4f; LON: %.4f; SATNUM: %d\n",
		gps->date,
		gps->time,
		gps->lat, gps->lon,
		gps->satnum
	);
}

int main()
{
	FILE *fp = fopen("gps_example.txt","r");
	char str[2048];
	size_t len = fread(str, 1, 2047, fp);
	str[len] = 0;

	gps_data_t gps;

	parse_gps(str, &gps);
	print_gps(&gps);

	return 0;
}