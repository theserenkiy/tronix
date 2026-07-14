

export default {
	props: ["type","message", "buttons"],
	data(){return{
		is_shown: 0,
		q: 0
	}},
	created()
	{
	},
	watch: {
		buttons(v)
		{
			console.log("ERROR",this.message,JSON.stringify(v))
		}
	},
	template: `
	<div :class="type">
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