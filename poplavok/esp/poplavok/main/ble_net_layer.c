#include <stdio.h>
#include <inttypes.h>
#include "ble_net_layer.h"
#include "ble_commands.h"

#define CMD_TIME	1
#define CMD_LED_ON	2


void ble_on_receive(void *buf, int len)
{
	uint8_t *cbuf = (uint8_t *)buf;
	uint8_t cmd_code = *cbuf;

	printf("COMMAND: %d\n",cmd_code);
	// printf("DATA: %s\n",((char *)buf)+1);
	
	ble_command_t *cmd;
	for(int i=0;;i++)
	{
		cmd = ble_commands + i;
		if(!cmd->code)
			break;
		if(cmd->code == cmd_code)
		{
			cmd->cb(cbuf+1, len-1);
			break;
		}
	}

	// 
}



