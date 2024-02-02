import Display from './display.mjs';
import {cl, apiCall, debounce, dbg} from '/lib.mjs';
import env from './env.mjs';
import {createText, createClip, createTextStrings} from './utils.mjs';
import Graphic from './graphic.mjs';

async function scenario0()
{
	let display = createClip({name:'DISPLAY',repeat:1});
		display.setSize(128,8)

		let t0 = createText('Я не пьяный, я - бухой! Мне всё похуй, панки - хой!!!','font_2');
		let t1 = createText('ПИЗДА','font_2');

		//let q = new Graphic({name:'Q',repeat:0})
		let q = createClip({name:'Q',repeat:1})
		//q.setSize(128,8)
		q.add(t0)
		//q.add(t1)

		t0.setPos(display.size[0],0)
		q.shot()
		for(var i=0;i < t0.size[0]+display.size[0];i++)
		{
			t0.shiftX(-1)
			//t0.shiftY(i%2 ? 1 : -1)
			//t1.shiftX(-1)
			q.shot()
		}

		//q.setFrame(9)

		display.add(q,0);
		display.shot(q.frames.length)
		//display.add(t1);

		display.reset()
		
		let frames = display.exportFrames()

		cl('frames',frames)

		for(let fr of frames)
		{
			this.image = fr;
			await new Promise(s=>setTimeout(s,50))
		}
		//this.image = q.render(1)
}


async function scenario1(img)
{
	let clip = createClip({size:[128,8]});
	let t0 = createText('Хуй\nПизда\nСковорода','font_2',{line_spacing: 9});
	clip.add(t0)
	t0.setPos(0,8)
	for(let i=0;i < t0.size[1];i++)
	{
		t0.shiftY(-1)
		clip.shot()
	}

	for(let i=0;i < 128;i++)
	{
		t0.shiftX(-1)
		clip.shot()
	}

	cl('clip frames',clip.frames)
	
	

	//this.image = gr.image
	//gr.draw(this.image)// = gr.image;
}

async function scenario2(img)
{
	let clip = createClip({size:[128,8]});
	
	let strs = createTextStrings('О'.split(''),'font_2')
	clip.add(strs)
	//clip.groupToLine(strs)

	// for(let i=0;i < strs.length;i++)
	// {
	// 	strs[i].pos[1] = i%2 ? 8 : -8
	// }

	// for(let j=0;j < 8;j++)
	// {
	// 	for(let i=0;i < strs.length;i++)
	// 	{
	// 		strs[i].shiftY(i%2 ? -1 : 1)
	// 	}
	// 	clip.shot()
	// }

	clip.shot();

	let an1 = strs[0].addAnimation('move',[
		{x:20,y:0,speed:1}
	])

	clip.shotAnimations(2)
	//an1.paused = true;
	strs[0].show = false;
	clip.shotAnimations(10)
	strs[0].show = true;
	an1.paused = false;
	clip.shotAnimations();
	dbg('frames',clip.frames.length)

	// clip.stepVectors();
	// clip.stepVectors();
	// clip.stepVectors();


	play(clip,this.image,100)
}

async function play(clip,image,delay=100)
{
	let frames = clip.exportFrames();

	for(let fr of frames)
	{
		image.splice(0,fr.length,...fr);
		await new Promise(s=>setTimeout(s,delay))
	}
}

export default {
	components: {
		Display
	},
	props: ['project','name'],
	data(){return{
		code: '',
		image: [3],
		console: [],
		saveDebounced: ()=>{},
	}},
	async created(){
		let d = await apiCall('readFile',{path:`/${this.project}/clips/${this.name}.js`});
		this.code = d.body;

		await this.run()

		//scenario0()
		scenario2.call(this)

		this.saveDebounced = debounce(async () => {
			await apiCall('writeFile',{
				path:`/${this.project}/clips/${this.name}.js`,
				data: this.code
			})
		},300)
	},
	watch: {
		code(s){
			cl('code changed')
			this.save()
		}
	},
	methods: {
		save(){
			this.saveDebounced();
		},
		log(s)
		{
			this.console.push({msg:s,type:'log'})
		},
		err(s)
		{
			this.console.push({msg:s,type:'err'})
		},
		async run(){
			this.log('Loading resources...');
			try{
				env.resources = await apiCall('loadResources',{project:this.project});
				this.log('Done!');
			}catch(err)
			{
				this.err(err)
			}
		}
	},
	template: `<div class="ed_clips">
		<Display width=128 height=8 :image="image" />
		<div>
			<button @click="run">Run!</button><br>
			<textarea v-model="code"></textarea>
			<div class=console>
				<p v-for="msg in console" v-html="msg.msg" :class="msg.type"></p>
			</div>
		</div>
	</div>`
}