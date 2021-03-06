package cc.gullinbursti.iterators {
	
	//] includes [!]>
	//]=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~.
	//]~=~=~=~=~=~=~=~=~=~=~=~=~=~[]~=~=~=~=~=~=~=~=~=~=~=~=~=~[

	// <[!] interface delaration [¡]>
	public interface IIterator {
	//]~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~._	
	
		//]~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~._
		function reset():void;
		//]~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~·¯
		
		
		//]~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~._
		function hasNext():Boolean;
		//]~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~·¯
		
		
		//]~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~._
		function next():Object;
		//]~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~·¯
		
		
		//]~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~._
		function length():int;
		//]~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~*~~·¯
	}
}