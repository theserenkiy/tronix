
const PaletteColors = {
    data(){return{
    }},
    mounted(){
        
    },
    computed: {
        cols(){
            let columns = []

            let hues = 12;
            let lights4hue = 6;
            let rows = 10;

            let lightStep = 100/(lights4hue+3)
            let satStep = 100/rows;
            let cols = hues*lights4hue;

            let hsls = []

            for(let row=0;row < rows;row++)
            {
                hsls.push([0,0,row >= rows/2 ? 0 : 100])
            }

            for(let hue=0;hue < 360;hue+=Math.floor(360/hues)){

                for(let lig=lightStep*2;lig < 100-lightStep;lig+=lightStep)
                {	
                    for(let sat=100;sat > 0;sat-=satStep)
                    {
                        hsls.push([hue,Math.round(sat),Math.round(lig)])
                    }
                }
            }

            let i=0;
            for(let col = 0;col < cols+1;col++)
            {
                let column = []
                for(let row=0;row < rows;row++){
                    let hsl = hsls[i++]
                    let color = `hsl(${hsl[0]}, ${hsl[1]}%, ${hsl[2]}%)`
                    column.push({color,rgb:hsl2rgb(...hsl)})
                }
                columns.push(column)
            }
            //cl(columns)
            return columns
        }
    },
    template: `
<div class="colors">
    <div v-for="col of cols">
        <div v-for="row of col" :style="{background:row.color}" :data-rgb="row.rgb" @click="$emit('color',row.rgb)"></div>
    </div>
</div>`
}