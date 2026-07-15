
import Battery from "./battery.vue.js"
import SignalStrength from "./signal_strength.vue.js"

const int = 1
const str = 2

const cmds = {
	set_time: [1, int],
	led_toggle: [2, str],
}

export default {

	props: ["device"],

	components: {
		Battery,
		SignalStrength
	},

	data(){return{
		charge: 50,
		dbm: -75,
		cmds: {}
	}},

	created()
	{
		// this.prepareCmds()
		this.cmds = cmds;

		this.sendCmd("led_toggle","Preved!")
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

		async sendCmd(cmd,data)
		{
			cl("SEND CMD",cmd)
			while(1)
			{
				if(this.device && this.device.is_connected)
					break
				await delay(100)
			}
			let cmd_prm = this.cmds[cmd]
			if(cmd_prm === undefined)
			{
				cl("UNKNOWN COMMAND "+cmd)
				return 0
			}
			let buf;
			if(cmd_prm[1] == int)
			{
				buf = new ArrayBuffer(data.length * 4 + 1);
				const view = new DataView(buf);
				
				view.setUint8(0, cmd_prm[0])

				for (let i = 0; i < data.length; i++) {
					view.setInt32(i*4 + 1, data[i], true); 
				}
			}
			else if(cmd_prm[1] == str)
			{
				const stringBuf = new TextEncoder().encode(data);

				buf = new Uint8Array(stringBuf.byteLength+1);

				buf.set(new Uint8Array([cmd_prm[0]]), 0);
				buf.set(stringBuf, 1);
			}

			cl("BUFFER",buf)
			await this.device.send(buf.buffer, cmd)
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
	</div>
	`
}