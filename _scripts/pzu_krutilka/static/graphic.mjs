import env from './env.mjs';
import { cl,dbg,warn } from './lib.mjs';


export default class Graphic
{
	name = '';
	isClip = false;
	isRenderable = true;

	uid = 0;
	image = [];	
	frames = [];
	objects = [];
	pos = [0,0];
	size = [0,0];
	width=0;
	height=0;
	offset=[0,0];
	logic='or';
	show=true;

	animations = {};
	is_animated = false;

	static uid_cnt = 1;

	constructor(opts={})
	{
		this.uid = Graphic.uid_cnt++;
		if(!opts.name)
			this.name = 'graphic_'+this.uid;
		else{
			this.name = opts.name.replace('$uid',this.uid)
		}
		let valid_params = this.copyOpts(opts)//,this.init_vars)		

		cl(`Init graphic object "${this.name}": ${JSON.stringify(valid_params)}`)

		if(opts.image)
			this.setImage(opts.image)
	}

	copyOpts(opts,keys)
	{
		let copied = {}
		for(let i in opts)
			if(!keys || keys.includes(i))
			{
				this[i] = opts[i];
				copied[i] = this[i];
			}
		
		return copied
	}
	
	setPos(x,y)
	{
		this.pos = [x,y]
	}

	setWidth(w)
	{
		this.size[0] = w;
	}

	setHeight(h)
	{
		this.size[1] = h;
	}

	setSize(w,h)
	{
		this.size = [w,h]
	}

	shiftX(value=1)
	{
		this.pos[0]+=value
	}

	shiftY(value=1)
	{
		this.pos[1]+=value
	}

	shift(x,y)
	{
		this.shiftX(x)
		this.shiftY(x)
	}

	setImage(image)
	{
		this.image = image;
		if(image.length < 1)return '';
		if(!this.size[0])
			this.setWidth(image.length);
		if(!this.size[1])
			this.setHeight(image[0] ? image[0].length : 0);
	}

	getImage(){return this.image}	

	add(obj,opts={})
	{
		if(Array.isArray(obj))
		{
			for(let o of obj)
			{
				this.add(o,opts)
			}
			return
		}
		opts = {
			logic: "OR",
			...opts
		}
		if(!this.isRenderable)
		{
			warn(`Cannot add object to "${this.name}". Object is not renderable.`)
			return
		}
		this.objects.push(obj)
		obj.parent = this;
		obj.logic = opts.logic;
		//return obj
	}

	groupToLine(objs,opts={})
	{
		opts = {
			spacing: 1,
			start_pos: [0,0],
			...opts
		}

		let x = opts.start_pos[0];
		for(let obj of objs)
		{
			obj.pos = [x,opts.start_pos[1]];
			x += obj.size[0]+opts.spacing;
		}
	}

	addAnimation(type,data,opts={})
	{
		if(!this['createAnimation_'+type])
			throw `Неправильный тип анимации "${type}"`
		
		let path = this['createAnimation_'+type].call(this,data,opts);
		let an = {type,path,cur:0,paused:false}
		cl(`Created animation: `,an)
		this.animations[type] = an;
		this.is_animated = true;
		return an;
	}

	createAnimation_blink(data,opts={})
	{
		for(let state of data)
		{

		}
	}

	createAnimation_move(data,opts={})
	{
		opts = {
			speed: 1,
			speedAxis: -1, 
			...opts
		}

		if(opts.speedAxis > 1)
		{
			opts.speedAxis = 0;
			cl(`Wrong speedAxis "${opts.speedAxis}". Set to 0.`)
		}
		let path = []
		let pos = this.pos
		for(let target of data)
		{
			target = target.map(v => Math.round(v))
			let delta = [target[0]-pos[0],target[1]-pos[1]]
			if(opts.speedAxis < 0)
				opts.speedAxis = delta[0]>delta[1] ? 0 : 1;
			
			let step_count = delta[opts.speedAxis];
			if(!step_count)
				return;
			let stepsize = [delta[0]/step_count,delta[1]/step_count];

			for(let i=0;i < step_count;i++)
			{
				pos = pos.map((v,i)=>v+stepsize[i])
				path.push(['pos',pos.map(v => Math.round(v))])
			}
		}
		return path;
	}

	removeAnimation(type)
	{
		if(!this.animations[type])
			return;
		
		delete this.animations[type];
		if(Object.keys(this.animations).length==0)
			this.is_animated = false
	}

	stepAnimation(type)
	{
		if(!this.animations[type])
		{
			warn(`Cannot step animation of type "${type}" on object "${this.name}"`);
			return;
		}
		return this.stepAnimationObject(this.animations[type])
	}

	stepAnimationObject(an)
	{
		let step = an.path[an.cur++];
		if(!step)
		{
			warn(`Animation ${an.type}@${this.name} completed`);
			return 0;
		}
		
		this[step[0]] = step[1];
		return 1;
	}

	stepAnimations()
	{
		let success_cnt = 0;
		for(let type in this.animations)
		{
			let an = this.animations[type];
			if(an.paused)
				continue;
			success_cnt += this.stepAnimationObject(an);
		}
		return success_cnt;
	}

	getFullOffset()
	{
		return this.pos.map((v,i) => this.offset[i]+v)
	}

	render()
	{
		if(!this.isRenderable)
			return;
		
		//dbg(`Rendering "${this.name}"`)

		//create list of children object's images and its offsets
		let imgs = []
		let min_offs = [Infinity,Infinity]

		//fill child imgs list and calc min_offset
		for(let obj of this.objects)
		{
			if(obj.isClip)
			{
				obj.setFrame(this.frame+obj.frameOffset)
				//obj.render()	//remove
			}
			else if(obj.isRenderable && obj.show)
				obj.render()

			if(!obj.show)
				continue;
			
			let ofs = obj.getFullOffset();
			imgs.push({im:obj.getImage(),ofs,logic:obj.logic.toLowerCase()});

			if(ofs[0] < min_offs[0])
				min_offs[0] = ofs[0]
			if(ofs[1] < min_offs[1])
				min_offs[1] = ofs[1]
		}

		this.offset = min_offs;

		//dbg(`"${this.name}" object images:`,imgs)
		//dbg(`"${this.name}" min offset:`,{min_offs})
		//imgs.sort((a,b) => (a.ofs[0] > b.ofs[0] || b.ofs[1] > b.ofs[1]) ? 1 : -1)

		if(!imgs)return 
		
		//create new empty image (2D array)
		//top-left corner (element [0,0]) has min_offs coordinate
		let rendered_image = []

		//write each object to image using `obj.logic` logic (OR/AND/XOR) for pixels
		for(let img of imgs)
		{
			if(!img.im.length)continue;

			//calculate img start offset relative to rendered_image
			let ofs = img.ofs.map((v,i) => v-min_offs[i]);
			//cl({ofs})

			//cl('Q',{ofs})
			for(let x=0;x < img.im.length;x++)
			{
				if(!rendered_image[x+ofs[0]])
					rendered_image[x+ofs[0]] = [];
				let col = rendered_image[x+ofs[0]];

				
				if(img.im[x]){ //sometimes empty elements are possible, so we must work around it
					//copying pixels
					//cl(this.name,x+ofs[0])
					for(let y=0; y < img.im[x].length; y++)
					{
						if(!col[y+ofs[1]])col[y+ofs[1]] = 0;
						let curval = col[y+ofs[1]];
						let newval = img.im[x][y];
						switch(img.logic)
						{
							case 'and': newval = newval && curval ? 1 : 0; break;
							case 'xor': 
							case 'eor': 
								newval = (newval || curval) && !(newval && curval) ? 1 : 0; break;
							default: newval = newval || curval;
						}
						col[y+ofs[1]] = newval;
					}
				}
			}
		}
		
		this.image = rendered_image
		return rendered_image;
	}

	draw(canvas)
	{
		let dummy_col = new Array(this.size[1]).fill(0);
		let dummy_img = new Array(this.size[0]).fill([]).map(v => [...dummy_col]);

		//cl({dummy_img})

		let xstart = this.offset[0] < 0 ? -this.offset[0] : 0;
		let ystart = this.offset[1] < 0 ? -this.offset[1] : 0;
		//cl({xstart})
		let image = [
			...(this.offset[0] > 0 ? dummy_img.slice(0,this.offset[0]) : []),
			...this.image.slice(xstart,-this.offset[0]+this.size[0]).map(col => {
				col = [
					...(this.offset[1] > 0 ? dummy_img[0].slice(0,this.offset[1]) : []),
					...col.slice(ystart,-this.offset[1]+this.size[1])
				]
				if(col.length < this.size[1])
					col.push(...new Array(this.size[1]-col.length).fill(0))
				return col;
			})
		]
		if(image.length < this.size[0])
			image.push(...dummy_img.slice(0,this.size[0]-image.length))

		canvas.splice(0,this.size[0],...image);
	}
}