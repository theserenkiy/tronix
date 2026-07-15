#include "ble_net.h"


//receive router
//needs ble_commands array!
void ble_on_receive(void *buf, int len)
{
	uint8_t *cbuf = (uint8_t *)buf;
	uint8_t cmd_code = *cbuf;

	printf("COMMAND: %d\n",cmd_code);
	// printf("DATA: %s\n",((char *)buf)+1);
	
	int res = -1;
	ble_command_t *cmd;
	for(int i=0;;i++)
	{
		cmd = ble_commands + i;
		if(!cmd->code)
			break;
		if(cmd->code == cmd_code)
		{
			res = cmd->cb(cbuf+1, len-1);
		}
	}
	if(res < 1)
		printf("ERROR: command %d not found\n", cmd_code);
	else
		printf("Command %d result: %d\n", cmd_code, res);
	
		
}