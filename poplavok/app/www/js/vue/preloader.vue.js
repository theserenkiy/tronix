

export default {
	props: ["message"],
	template: `
	<div class="preloader">
		<div class="central">
			<div class="message" v-html="message"></div>
			<div class="loader loader1"></div>
		</div>
	</div>
	`
}