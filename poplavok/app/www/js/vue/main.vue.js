import WBLE from "../ble.js"
import Preloader from "./preloader.vue.js"
import Error from "./error.vue.js"

const DEV_NAME = "POPLAVOK"
const SERVICE_UUID_ARRAY =	[0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00]
const CHAR_UUID_ARRAY = 	[0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x01]

export default {
	components: {
		Preloader,
		Error
	},
	data(){return{
		wble: null,
		preloader_shown: 1,
		preloader_message: "",
		error_shown: 0,
		error_message: "",
		error_buttons: [],
		error_btn_pressed: -1
	}},

	async created()
	{
		// await this.error("Ошибочка вышла",["Ok?"])
		const wble = new WBLE()
		this.wble = wble
		this.wble.addListener(this.onBLE)
		this.btInit()		
	},

	async mounted(){

	},

	methods: {

		onBLE(code, msg, level)
		{
			let is_error = level=="error" ? 1 : 0
			let [module, event] = code.split(".")
			//clog(`${level.toUpperCase()}: [${code}] ${JSON.stringify(msg)}`, lvl)
			if(is_error)
			{
				if(msg.dev_id !== undefined)
					this.onBLE_device_error(module, event, msg.msg, msg.dev_id)
				else
					this.onBLE_error(module,event,msg)
			}
			
		},

		onBLE_error(module, event, msg)
		{
			switch(module)
			{
				case "bt":
					return this.error(msg, [["Повторить", ()=>this.btInit()]])
				
				case "scan":
					return this.error(msg, [
						["Повторить", ()=>this.scan()]
				])

				
			}
		},

		onBLE_device_error(module, event, msg, devid)
		{
			switch(module)
			{
				case "connect":
					return this.error(msg, [
						["Подключиться заново", ()=>this.connect()]
					])
			}
		},

		preloaderShow(msg)
		{
			this.preloader_shown = 1
			this.preloader_message = msg
		},

		preloaderHide()
		{
			this.preloader_shown = 0
		},

		async error(msg,buttons)
		{
			this.error_message = msg
			this.error_buttons = buttons
			this.error_shown = 1
			this.error_btn_pressed = -1
			while(1)
			{
				await delay(100)
				if(this.error_btn_pressed >= 0){
					this.error_shown = 0
					await delay(100)
					return this.error_btn_pressed
				}
			}
		},

		onErrorButton(ev)
		{
			this.error_shown = 0
			ev()
		},

		async btInit()
		{
			this.preloaderShow("Запускаем Bluetooth...")
			let res = await this.wble.init(
				DEV_NAME,
				SERVICE_UUID_ARRAY,
				CHAR_UUID_ARRAY			
			)

			if(res.level != "error")
				this.scan()
		},

		async scan()
		{
			this.preloaderShow("Поиск устройства...")
			let res = await this.wble.scan(10)

			if(res.level!="error")
				this.connect()
		},
		
		async connect()
		{
			this.preloaderShow("Устройство найдено, подключаемся...")
			let dev = this.wble.devices[0]

			let res = await dev.connect()
			cl("CONNECT RES",res)
			if(res.level != "error")
				cl("CONNECTED!")
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
			:message="preloader_message"
		></Preloader>
		<Error
			v-show="error_shown"
			:message="error_message"
			:buttons="error_buttons"
			@onButton="onErrorButton"
		></Error>
	</div>
	`
}
