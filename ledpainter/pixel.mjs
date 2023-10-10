
const Pixel = {
	props:['color','cur_color','num'],
	data(){return{
		isOver: false
	}},
	methods:{
		mOver(ev){
			cl('over')
			this.isOver = true
			if(ev.buttons==1){
				this.$emit('set',this.num)
			}
		},
		mOut(){
			this.isOver = false
		},
		click(){
            
        }

	},
	computed: {
		actualColor(){
			return cssColor(this.isOver ? this.cur_color : this.color)
		}
	},
	template: `
<div @mouseenter="mOver" @mouseleave="mOut" @click="$emit('set',num)" draggable="false">
	<div draggable="false">
		<div :style="{backgroundColor:actualColor}" draggable="false"></div>
	</div>
</div>`
}