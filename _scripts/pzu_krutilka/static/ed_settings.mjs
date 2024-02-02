

export default {
    props: [],
    data(){return{
        q:0
    }},
    created(){
        this.q = Date.now();
    },
    template: `<p>I'm a setted, launched @{{q}}</p>`
}