

export default {

	props: ["dbm"],

	computed: {
		signal_class()
		{
			return "signal-"+(() => {
				if(this.dbm > -60)return 4
				if(this.dbm > -70)return 3
				if(this.dbm > -85)return 2
				if(this.dbm > -95)return 1
				return 0
			})()
		}
	},

	template: `
	<div class="signal-indicator" :class="signal_class">
		<div class="bar bar-1"></div>
		<div class="bar bar-2"></div>
		<div class="bar bar-3"></div>
		<div class="bar bar-4"></div>
	</div>

	`
}