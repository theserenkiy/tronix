

const cmds = {
	set_time: 1,
	led_toggle: 2,
}


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
			let colors = ["off","green","red"]
			let code = colors.indexOf(v)
			if(code < 0)code = 0
			this.sendCmd("led_toggle",[["uint8", [code]]])
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