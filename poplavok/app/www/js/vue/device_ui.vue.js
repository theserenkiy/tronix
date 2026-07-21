

const cmds = {
	get_state: 1,
	set_time: 2,
	set_led: 3,
}

const led_colors = ["off","green","red"]

export default {
	props: ["rcvd_data"],

	data(){return{
		led_color: "off"
	}},

	methods: {
		sendCmd(cmd,struct)
		{
			cl("SEND CMD",cmd)
			
			let cmd_code = cmds[cmd]
			if(cmd_code === undefined)
			{
				cl("UNKNOWN COMMAND "+cmd)
				return 0
			}
			
			this.$emit("send",[
				["uint8",[cmd_code]],
				...struct
			])
		}
	},

	watch: {
		led_color(v)
		{
			let code = led_colors.indexOf(v)
			if(code < 0)code = 0
			this.sendCmd("set_led",[["uint8", [code]]])
		},

		"rcvd_data.led"(v)
		{
			this.led_color = led_colors[v]
		}
	},

	template: `
	<div class="device_ui">
		<div class="row">
			<select v-model="led_color">
				<option value="off">OFF</option>
				<option value="red">RED</option>
				<option value="green">GREEN</option>
			</select>	
		</div>
	</div>
	`
}