import Pixel from '/pixel.mjs'
import env from './env.mjs'
import { cl, debounce } from './lib.mjs'

const drawareas_registry = {};
const cur_drawarea_regid=0;

export default {
    components: {
        Pixel
    },
    props: ['width','height','image','readonly'],
    data(){
        return {
            pixels: [],
            env,
            regid:0,
            height_bytes: 0
        }
    },
    created(){
        this.debouncedSave = debounce(()=>{
            let img = this.image || [];
            for(let px of this.pixels)
            {
                let {img_index,bit_number,mask} = this.getBitAddress(px.addr);
                //cl({px})
                if(undefined===img[img_index])
                    img[img_index] = 0;
                
                img[img_index] &= mask
                img[img_index] |= px.value << bit_number;
            }
            this.$emit('change');
            //this.saved = this.pixels.filter(v => v.value).map(v => v.addr)
        },100)
        this.init();
    },
    watch:{
        width(){
            this.updatePixels();
            this.debouncedSave();
        },
        image(){
            if(!this.readonly)
                return;
            this.updatePixels();
        }
    },
    computed:{
        style(){
            let s = this.env.settings;
            let w = this.width*s.editor_pixel_fullsize;
            let h = this.height*s.editor_pixel_fullsize;
            let style = {width:w+'px',height:h+'px'};
            return style;
        }
    },
    methods:{
        getBitAddress(addr)
        {
            let img_index = (addr[0]*this.height_bytes)+Math.floor(addr[1]/8);
            let bit_number = addr[1]%8;
            let mask = ~(1 << bit_number);
            return {img_index,bit_number,mask}
        },
        focus(){
            cur_drawarea_regid = this.regid;
        },
        init(){
            this.height_bytes = Math.ceil(this.height/8);
            this.updatePixels();
            this.resize();
            this.regid = Date.now();
        },
        updatePixels(){
            this.pixels = [];
            for(let i=0;i < this.width*this.height;i++)
            {
                let addr = [i%this.width,Math.floor(i/this.width)]
                let {img_index,bit_number,mask} = this.getBitAddress(addr);
                let value = this.image[img_index] 
                    ? (this.image[img_index] & (1 << bit_number) ? 1 : 0) 
                    : 0;
                this.pixels.push({value,addr})
            }
        },
        resize(){
            
        },
        onPixel(ev){
            this.focus = true;
            this.pixels[ev.num].value = ev.value;
            this.debouncedSave()
        }
    },
    template: `<div class=drawarea :style="style">
        <Pixel v-for="(pixel,num) of pixels" 
            :addr="pixel.addr" 
            :num="num" 
            :value="pixel.value" 
            :key="pixel.addr"
            :size="env.settings.editor_pixel_fullsize"
            :padding="env.settings.editor_pixel_padding"
            :readonly="readonly"
            @set="onPixel">
        </Pixel>
    </div>`
}