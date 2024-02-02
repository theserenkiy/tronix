import {cl} from '/lib.mjs';

export default {
	props: ['value','num','addr','size','padding','readonly'],
	data(){
		return {
			
		}
	},
	created(){
		this.resize()
	},
	watch:{
		
	},
	computed:{
		style(){
			return {
				width:this.size+'px',
				height:this.size+'px',
				padding:this.padding+'px'
			}
		}
	},
	methods:{
		resize(){
			
		},
		mOver(ev){
			if(this.readonly)
				return;
			cl('over',ev.buttons)
			this.isOver = true;
			if(ev.buttons>0){
				this.$emit('set',{num:this.num,value:ev.buttons==1})
			}
		},
		mOut(){
			this.isOver = false
		},
		click(ev){
			if(this.readonly)
				return;
			cl(ev.button)
			this.$emit('set',{num:this.num,value:ev.button==0})
			return false;
		}
	},
	template: `
	<div class="pixel nodrag noselect" 
		draggable="false"
		:class="{active:value}" 
		:style="style" 
		@mousedown="click"
		@mouseenter="mOver" 
		@mouseleave="mOut"
		@dragstart.prevent.stop=""
	></div>`
};