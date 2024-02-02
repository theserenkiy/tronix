
export default class Animation
{
	validTypes = ['move','blink','resize'];
	chain = []

	constructor(type,data)
	{
		this.add(type,data,opts)
	}

	addSeries(type,data)
	{
		if(typeof type !== 'string')
			throw `При создании анимации, первый аргумент (type) должен быть строкой`;

		if(!this.validTypes.includes(type))	
			throw `Неподдерживаемый тип анимации "${type}". Поддерживаемые типы: ${this.validTypes.join(', ')}`
	
		let an = this['compile_'+type].call(this,data,opts)
	}

	compile_move(data,opts)
	{

	}


}