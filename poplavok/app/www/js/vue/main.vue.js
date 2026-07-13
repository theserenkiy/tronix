import WBLE from "./ble.js"
import Preloader from "./preloader.vue.js"

const DEV_NAME = "POPLAVOK"
const SERVICE_UUID_ARRAY =	[0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00]
const CHAR_UUID_ARRAY = 	[0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x01]

export default {
	components: {
		Preloader
	},
	data(){return{
		wble: null,
		preloader_shown: 1
	}},

	async created(){
		const wble = new WBLE()

		app.wble = wble

		wble.addListener(this.onBLE)

		if(!await wble.init(
			DEV_NAME,
			SERVICE_UUID_ARRAY,
			CHAR_UUID_ARRAY			
		))
			throw "Cannot connect to Bluetooth"
		
		await wble.scanUntilSuccess()
		
		let dev = wble.devices[0]
		if(await dev.connect())
		{
			// for(let i = 0;;i++)
			// {
			// 	dev.send("Hello world! "+i,"hello",1)
			// 	await delay(1000)
			// }
		}
	},

	async mounted(){

	},

	methods: {

		onBLE(code, msg, level)
		{
			let lvl = level=="error" ? 2 : 0
			//clog(`${level.toUpperCase()}: [${code}] ${JSON.stringify(msg)}`, lvl)
		}

	},
	watch:{

	},
	computed: {

	},

	template: `
	<div class="app">
		<Preloader
			v-show="preloader_shown"
		></Preloader>
	</div>
	`
}
