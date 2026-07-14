

export default {

	props: ["charge"],

	computed: {
		charge_width()
		{
			return this.charge > 40 ? this.charge : 10+(this.charge*0.75)
			// 	this.charge > 20 ? 25+(this.charge*0.75) : 10+(this.charge*0.75)
			// )
		},

		charge_color()
		{
			return this.charge > 25 
				? "#0c0"
				: (this.charge > 10 ? "#ea0" : "#e00")
		}
	},

	template: `
	<div class="battery" :class="{blinking: charge < 10}">
		<div 
			class="charge-level"
			:style="{'background-color':charge_color, width:charge_width+'%'}"
		></div>
		<span v-html="charge"></span>
	</div>
	`
}