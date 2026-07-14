

export default {
	props: ["message", "buttons"],
	data(){return{
		is_shown: 0,
		q: 0
	}},
	created()
	{
		console.log("BUTTONS",this.buttons)
	},
	watch: {
		buttons(v)
		{
			console.log("BUTTONS",v)
		}
	},
	template: `
	<div class="error">
		<div class="central">
			<div class="message" v-html="message"></div>
			<div class="buttons">
				<button 
					v-for="(but,i) in buttons" 
					v-html="but[0]"
					@click="$emit('onButton',but[1])"
				></button>
			</div>
		</div>
	</div>
	`
}