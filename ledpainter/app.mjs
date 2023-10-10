const app = {
	components: {
		PaletteColors,
		Pixel
	},
	data(){return{
		panel:[],
		cur_color: [0,0,0],
		pixels: [],
		export_use_db: true,
		export_decimal: false,
		size: [5,5]
	}},
	mounted(){
		
		this.updatePixels()

		this.sendUART = debounce(()=>{
			cl('sending to UART')	
		},500)
	},
	computed:{
		cur_color_css(){
			return cssColor(this.cur_color)
		},
		dump(){
			let chunks = []
			for(let i=0;i < this.pixels.length;i+=4)
			{
				chunks.push(this.pixels.slice(i,i+4))
			}
			return chunks.map(ch => (this.export_use_db ? '.db ' : '')+ch.map(
					pix => pix.map(
						c => this.export_decimal ? c+'' : '0x'+c.toString(16).toUpperCase().padStart(2,'0')
					).join(', ')
				).join(', ')
			).join('\n')
		},
		pixSize(){
			return this.size[0] <= 20 ? 30 : 10;
		}
	},
	methods:{
		updatePixels()
		{
			this.pixels = []
			this.$refs.panel.style.width = (30*this.size[0])+'px';
			for(let row = 0;row < this.size[1];row++)
			{
				for(let col=0;col < this.size[0];col++)
				{
					this.pixels.push([0,0,0])
				}
			}
		},
		onColor(col){
			cl(col)
			this.cur_color = col
		},
		setPixel(num){
			this.pixels[num] = this.cur_color
		},
		loadDump(dump){
			//cl('load dump',dump)
			let rex = /(0x[\da-f]{2})/gi
			let m,bytes=[]
			while(m = rex.exec(dump))
			{
				bytes.push(parseInt(m[1]))
			}
			cl({bytes})
			this.pixels = []
			for(let i=0;i < bytes.length;i+=3){
				this.pixels.push([
					bytes[i],
					bytes[i+1] || 0,
					bytes[i+2] || 0,
				])
			}
			cl(this.pixels)
		}
	},
	watch:{
		pixels: {
			handler(){
				this.sendUART()
			},
			deep:true
		},
		size: {
			handler(sz){
				if(sz[0] > 64 || sz[1] > 64)
				{
					alert('Max size: 64 pix by any axis')
					this.size = sz.map(v => v > 64 ? 64 : v)
				}
				this.updatePixels()
			},
			deep:true
		}
	},
	template: `
<div>
	<div class="panelarea">
		<div class="panel" draggable="false" ref="panel" :style={width:(size[0]*pixSize)+'px'}>
			<Pixel v-for="(color,i) of pixels" 
				draggable="false"
				:color="color" 
				:cur_color="cur_color"
				:num="i"
				:style={width:pixSize+'px',height:pixSize+'px'}
				@set="setPixel"
				@dragstart.prevent=""
			></Pixel>
		</div>
	</div>
	<div class=size>
		Panel size: <input type=number v-model=size[0] min=1 max=64> x <input type=number v-model=size[1] min=1 max=64>
	</div>
	<div class="palette">
		<div class="color_info">
			<div class="color" :style="{backgroundColor:cur_color_css}"></div>
			<div class=rgb>
			<label>R:<input :value="cur_color[0]"></label>
			<label>G:<input :value="cur_color[1]"></label>
			<label>B:<input :value="cur_color[2]"></label>
			</div>
		</div>
		<PaletteColors @color="onColor"/>
	</div>
	
	<textarea :value=dump @change="loadDump($event.target.value)"></textarea>
	<div class="export_sett">
		<label><input type=checkbox v-model="export_use_db">Add .db</label>
		<label><input type=checkbox v-model="export_decimal">DEC/HEX</label>
	</div>
</div>`
}