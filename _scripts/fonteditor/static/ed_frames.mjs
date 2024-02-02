import Drawarea from '/drawarea.mjs';
import env from '/env.mjs';

export default {
    components:{
        Drawarea
    },
    props: [],
    data(){return{
        env
    }},
    created(){
    },
    template: `
    <div>
        <Drawarea :width="env.settings.frame_width_dots" :height="env.settings.frame_height_dots"/>
    </div>
    `
}