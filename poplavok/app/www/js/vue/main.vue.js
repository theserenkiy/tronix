import WBLE from "../ble.js"
import Preloader from "./preloader.vue.js"
import Popup from "./popup.vue.js"
import Device from "./device.vue.js"
import { warn } from "../vue-esm.js"

const DEV_NAME = "POPLAVOK"
const SERVICE_UUID_ARRAY	= [0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00]
const CHAR_UUID_ARRAY		= [0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x01]

export default {
	components: {
		Preloader,
		Popup,
		Device
	},
	data(){return{
		wble: null,
		preloader_shown: 0,
		preloader_message: "",
		error_shown: 0,
		error_message: "",
		error_buttons: [],
		notify_level: "notify",
		notify_shown: 0,
		notify_message: "",
		notify_buttons: [],
		notify_timer: null,
		device: null,
		view: "device"
	}},

	async created()
	{
		// await this.error("Ошибочка вышла",["Ok?"])
		const wble = new WBLE()
		this.wble = wble
		this.wble.addListener(this.onBLE)
		// this.btInit()
		this.notify("Какое-то предупреждение",[["Отмена"]],2)
	},

	async mounted(){

	},

	methods: {

		onBLE(code, msg, level)
		{
			let is_error = level=="error" ? 1 : 0
			let [module, event] = code.split(".")
			clog(`${level.toUpperCase()}: [${code}] ${JSON.stringify(msg)}`, level)

			let lvl = {[level]:1}

			if(msg.dev_id !== undefined)
				this.onBLE_device_event(module, event, lvl, msg.msg, msg.dev_id)
			else
				this.onBLE_event(module, event, lvl, msg)
		},

		onBLE_event(module, event, lvl, msg)
		{

			switch(module)
			{
				case "bt":
					if(lvl.error)
						return this.error(msg, [
							["Повторить", ()=>this.btInit()],
							["Отмена", ()=>this.btCancel()]
						])
				
				case "scan":
					if(lvl.error)
						return this.error(msg, [
							["Повторить", ()=>this.scan()],
							["Отмена", ()=>this.btCancel()]
				])
			}
		},

		onBLE_device_event(module, event, lvl, msg, devid)
		{
			switch(module)
			{
				case "connect":
					if(lvl.error)
						return this.error(msg, [
							["Подключиться заново", ()=>this.connect()],
							["Отмена", ()=>this.btCancel()]
						])
					else if(event=="lost")
						return this.warn(`Связь потеряна. Переподключаемся`,[],10)
					else if(event == "ok")
						return this.notify(`Связь установлена!`,[],3)
				
				case "reconnect":
					if(event=="in")
						return this.warn(`Связь потеряна. Повтор через ${msg} сек.`, [
							["Отмена", ()=>this.btCancel()]
						], 2)
				
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
		},

		async onErrorButton(ev)
		{
			this.error_shown = 0
			await delay(100)
			ev()
		},

		async warn(msg,buttons,timeout=0)
		{
			return this.notify(msg,buttons,timeout,"warn")
		},

		async notify(msg,buttons,timeout=0,level="notify")
		{
			if(this.notify_timer)
				clearTimeout(this.notify_timer)
			this.notify_timer = null

			this.notify_level = level
			this.notify_message = msg
			this.notify_buttons = buttons
			this.notify_shown = 1
			
			if(timeout)
				this.notify_timer = setTimeout(()=>this.notify_shown=0, timeout*1000)

		},

		onWarnButton(ev)
		{
			this.notify_shown = 0
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

		async btCancel()
		{
			this.wble.stop()
			this.switchView("main")
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
			{
				this.device = dev
				this.switchView("device")
			}
		},

		switchView(name)
		{
			this.preloaderHide()
			this.view = name
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
		<Popup
			type="error"
			v-show="error_shown"
			:message="error_message"
			:buttons="error_buttons"
			@onButton="onErrorButton"
		></Popup>
		<Popup
			:type="notify_level"
			v-show="notify_shown"
			:message="notify_message"
			:buttons="notify_buttons"
			@onButton="onErrorButton"
		></Popup>
		<Device
			v-if="view=='device'"
			:device="device"
		></Device>
		<div 
			v-if="view=='main'"
			class="view"
		>
			<div class="central">
				<button @click="btInit()">Включить Bluetooth</button>
			</div>
		</div>
	</div>
	`
}
