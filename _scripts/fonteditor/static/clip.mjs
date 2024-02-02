import env from './env.mjs';
import { cl, dbg } from './lib.mjs';
import Graphic from './graphic.mjs';

export default class Clip extends Graphic
{
	//public
	speed = 1
	repeat = 0	//0 = forever

	//private
	isClip = true
	frames = []
	frame = 0
	repeated = 0
	frameOffset = 0
	
	constructor(opts)
	{
		super(opts)
		super.copyOpts(opts,'speed,repeat'.split(','))
	}

	add(obj,opts={})
	{
		opts = {
			startFrame: 0,
			...opts
		}
		super.add(obj,opts)
		obj.frameOffset = this.frame+opts.startFrame;
	}

	reset()
	{
		this.setFrame(0)
	}

	nextFrame()
	{
		this.setFrame(this.frame+1)
	}

	setFrame(frame)
	{
		this.frame = frame;
		let num_ = frame%this.frames.length;
			
		if(this.repeat > 0 && frame/this.frames.length >= this.repeat)
		{
			warn(`Запрошенный кадр #${num} клипа "${this.name}" не существует.`)
			this.image = []
		}
		else {
			let fr = this.frames[num_]
			this.image = fr[0]
			this.offset = fr[1]
		}
		//this.image = this.frames[num]
	}

	shot(num=1)
	{
		for(let o of this.objects)
		{
			if(!o.is_animated)continue;
			o.stepAnimations();
		}
		this.renderFrames(num);
	}

	renderFrames(num=1)
	{
		for(let i=0;i < num;i++)
		{
			this.render();
			let imcopy = [...this.image].map(col => col ? [...col] : []);
			this.frames[this.frame] = [imcopy,[...this.offset]];
			this.frame++;
		}
	}

	shotAnimations(frames=0,objs=null)
	{
		let frame=0;
		for(;frame < (frames || env.settings.total_frames);frame++)
		{
			if(!objs)objs = this.objects;
			let success_cnt = 0;
			for(let o of objs)
			{
				if(!o.is_animated)
					continue;
				success_cnt += o.stepAnimations();
			}
			if(success_cnt)
				this.renderFrames(1);
			else break;
		}
		cl('Frames animated: '+frame);
	}

	exportFrames()
	{
		let list = [];
		for(let i = 0;i < this.frames.length;i++)
		{
			this.setFrame(i);
			//dbg('setframe '+i,this.image)
			list[i] = []
			this.draw(list[i])
		}
		return list;
	}
}