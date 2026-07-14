
import Battery from "./battery.vue.js"
import SignalStrength from "./signal_strength.vue.js"

export default {

	props: ["device"],

	components: {
		Battery,
		SignalStrength
	},

	data(){return{
		charge: 50,
		dbm: -75
	}},

	async mounted()
	{
		while(1)
		{
			this.dbm = this.device.rssi
			await delay(500)
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
				<div class="disconnect"></div>
			</div>
		</div>
	</div>
	`
}