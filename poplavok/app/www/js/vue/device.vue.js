
import Battery from "./battery.vue.js"
import SignalStrength from "./signal_strength.vue.js"
import DeviceUI from "./device_ui.vue.js"
import pack from "../pack.js"

export default {

	props: ["device"],

	components: {
		Battery,
		SignalStrength,
		DeviceUI
	},

	data(){return{
		charge: 50,
		dbm: -75,
		cmds: {},
		rcvd_data: {}
	}},

	created()
	{
		
	},
	
	async mounted()
	{
		while(1)
		{
			if(this.device)
				this.dbm = this.device.rssi
			await delay(500)
		}
	},

	methods: {
		prepareCmds()
		{
			let lines = cmds_header.split("\n")
			for(let l of lines)
			{
				l = l.trim()
				if(!l || l[0]=="/")
					continue
				let vv = s.split(/\s+/g)
				if(vv.length < 3)
					continue
				this.cmds[vv[1].replace("CMD_","")] = +vv[2]
			}
		},

		async sendCmd(struct)
		{
			return await this.device.sendStruct(struct)
		}
	},

	watch:{
		async device(dev)
		{
			if(dev)
			{
				let res = await this.sendCmd()
			}
		}
	},

	template: `
	<div class="device">
		<div class="topbar">
			<div class="title">Device</div>
			<div class="indicators">
				<div class="charge">
					<Battery :charge="charge"></Battery>
				</div>
				<div class="signal">
					<SignalStrength :dbm="dbm"></SignalStrength>
				</div>
				<div class="disconnect">
					<i class="icon-power" @click="$emit('disconnect',device)"></i>
				</div>
			</div>
		</div>
		<DeviceUI
			:rcvd_data="rcvd_data",
			@send="sendCmd"	
		></DeviceUI>
	</div>
	`
}