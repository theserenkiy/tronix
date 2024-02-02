function main(ctx)
{
    stroka1 = ctx.createClip(stroka)
    
    while(ctx.framesLeft() > stroka1.length)
    {
        stroka.play()
    }
}


function stroka(ctx)
{
   var text = ctx.createText('Привет, мир!',{font:'font1',letter_space:2});
   
   text.pos(ctx.width,0);
   while(text.moveTo(-text.width,0))
   {
       ctx.render()
   }
}



