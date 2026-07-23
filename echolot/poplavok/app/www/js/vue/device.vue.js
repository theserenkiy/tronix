
import Battery from "./battery.vue.js"
import SignalStrength from "./signal_strength.vue.js"
import DeviceUI from "./device_ui.vue.js"
import { unpack } from "../pack.js"

const rcv_struct = [
	[1,[
		["uint8",1,"led"]
	]]
]


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
		this.initDevice()
		
	},

	methods: {
		
		async initDevice()
		{
			if(!this.device)return
			this.device.onRSSI = rssi => this.dbm = rssi
			this.device.onData = buf => this.receiveData(buf)
		},

		async sendCmd(struct)
		{
			return await this.device.sendStruct(struct)
		},

		async receiveData(buf)
		{
			// cl("LEN",buf.byteLength)
			let cmd = new Uint8Array(buf,0,1)[0]
			// cl("VIEW",view)

			let struct = rcv_struct.find(v => v[0]==cmd)
			if(!struct)
			{
				cl("CANNOT FIND STRUCT FOR CMD ",cmd)
				return
			}

			let data = unpack(buf, struct[1], 1)

			cl("DATA", data)
			this.rcvd_data = {...this.rcvd_data, ...data}
		}
	},

	watch:{
		async device(dev)
		{
			this.initDevice()
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
			:rcvd_data="rcvd_data"
			@send="sendCmd"	
		></DeviceUI>
	</div>
	`
}