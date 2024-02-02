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
            height_bytes: 0,

            pixel_size: 15,
            pixel_padding: 1
        }
    },
    created(){
        this.debouncedSave = debounce(()=>{
           
            this.$emit('change',this.pixels);

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
            let fullsz = this.pixel_size+this.pixel_padding;
            let w = this.width*fullsz;
            let h = this.height*fullsz;
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

            for(let y=0; y < this.height; y++)
            {
                for(let x=0; x < this.width; x++)
                {
                    this.pixels.push({
                        value: this.image[y] ? (this.image[y][x] || 0) : 0, 
                        addr: [x,y]
                    });
                }
            }
        },
        resize(){
            
        },
        onPixel(ev){
            this.focus = true;
            this.pixels[ev.addr[1]*this.width+ev.addr[0]] = ev.value;
            this.debouncedSave()
        }
    },
    template: `<div class=drawarea :style="style">
        <Pixel v-for="pixel in pixels" 
            :addr="pixel.addr" 
            :value="pixel.value" 
            :size="pixel_size"
            :padding="pixel_padding"
            :key="pixel.addr"
            @set="onPixel">
        </Pixel>
    </div>`
}