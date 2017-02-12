# Enhanced-Apex-Simulator-
Implemented 5 stage pipe line. Fetch, Decode, Centralized Issue Queue. Branch prediction and forwarding. 

Enhanced simulator	to	perform	out-oforder	execution.		

Pipeline	stages	All	instructions	will	first	go	through	the	following	three	states	in	the	in-order	frontend:	• Fetch	• Decode	/	Rename	
• Centralized	Issue	Queue		Instructions	will	leave	the	IQ	when	dependencies	are	resolved	and	enter	one	of	the	following	functional	units:	
• INT	–	Integer	operations,	e.g.	AND,	SUB,	etc.		Also	performs	branch	resolution.		One	cycle	of	latency.	
• LSQ	–	Memory	access,	including	address	calculation,	and	forwarding	through	memory.		3	cycles	latency	(the	first	stage	computes	the	address).	
• MUL	–	Integer	multiply,	4	cycles	latency.		You	will	also	implement	a	reorder	buffer	and	precise	exceptions.		Branches	You	will	use	sign-based	static	branch	prediction,	where	negative	offsets	are	predicted	taken	and	positive	offsets	assumed	not-taken.	
	Mispredicted	branches	will	be	handled	like	exceptions,	waiting	for	the	head	of	the	RoB	to	catch	up	to	the	branch	instruction.		
  
  Forwarding	
  • Use	tag-based	forwarding.	
  • Forward	to	IQ	entries	and	to	Decode.	
  • The	simulator	should	behave	as	though	the	tag	precedes	the	result	by	one	cycle	so	that	back-to-back	dependencies	can	execute	on	consecutive	cycles.		Arbitration	for	Completion	When	multiple	functional	units	complete	on	the	same	cycle,	only	one	must	be	allowed	to	forward.		Others	must	stall.		
  
  Priority	from	highest	to	lowest	is:	
  • LSQ	
  • INT	
  • MUL		Memory	and	LSQ	The	“LSQ”	unit	encompasses	the	functionality	of	the	load-store	queue,	load-store	unit,	address	calculation,	and	access	to	main	memory.		(In	this	project,	we	do	not	implement	a	cache.)		It	has	the	following	latencies:	
  • Completing	a	load	from	main	memory	takes	3	cycles.	
  • Completing	a	load	forwarded	from	a	store	(with	known	data)	in	the	LSQ	takes	3	cycles.	
  • If	a	load	depends	on	(has	the	same	address	as)	a	store	with	unknown	data,	then	there	is	a	3-cycle	latency	from	the	time	the	store’s	data	dependency	is	satisfied	until	when	the	load	can	complete.		
  
  You	will	have	a	four-entry	load-store	queue.		
  Store	instructions	stay	in	the	queue	until	they	retire,	because	we	do	not	update	memory	until	retirement.		Load	instructions	can	leave	the	LSQ	at	completion	(but	must	stay	in	the	RoB	until	retirement).		
  
  The	LSQ	is	a	proper	queue,	where	instructions	enter	the	tail	on	dispatch	and	leave	the	head	on	retirement.				
  You	must	keep	track	of	“unknowns”	for	load	and	store	instructions. A	load	can	have	an	unknown	address.		
  
  A	store	can	have	an	unknown	address,	and	it	can	have	unknown	data.		These	will	be	unknown	when	a	dependency	has	not	completed	yet.		The	disambiguation	algorithm	is	as	follows:	
  • If	the	LSQ	is	empty,	a	load	will	access	the	data	memory	and	complete.	• Starting	from	the	head	of	the	queue	and	iterating	backwards,	compare	each	load’s	address	to	store	addresses	that	precede	it	in	program	order.			

If	a	matching	store	address	is	found,	stop	iterating	and	complete	the	load	through	forwarding	from	the	store.	o If	an	unknown	address	is	found,	stop	iterating	and	stall	the	load.	
If	all	preceding	store	addresses	are	known	but	none	match,	the	load	will	complete	by	accessing	the	data	memory.		Speculative	loads	(hoisting)	are	not	to	be	supported.		
Other	notes	
• The	HALT	instruction	does	not	take	effect	until	it	retires.	
