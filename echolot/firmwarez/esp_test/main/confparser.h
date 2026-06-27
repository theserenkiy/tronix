#pragma once

void confparser_init();

void confparser_next(int direction);

char *confparser_get_name();

void confparser_get_config(int n, int cfreq);

void confparser_test_summary(char *s);
