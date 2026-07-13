
import { createApp } from "./vue-esm.js";
import VueMain from "./vue/main.vue.js"

export async function initApp() {
	try{
		clog('Cordova готова. Проверяем Bluetooth...');


		const App = {
			components:{
				VueMain
			},
			template: `
				<VueMain></VueMain>
			`
		}
		
		const app = createApp(App)
		app.mount(document.body);
		
		

			

	}catch(e)
	{
		cerror("Fatal error: "+e)
	}
}

