''
'' Threaded producer/consumer example using a one-item buffer.
''
'' The mutex protects item_ready and shutdown_requested. Each condition is
'' checked in a While loop because a condition wait may wake spuriously.
''

const ITEM_COUNT = 10

type PRODUCER_CONSUMER_STATE
	lock as any ptr
	can_produce as any ptr
	can_consume as any ptr
	item_ready as integer
	shutdown_requested as integer
end type

declare sub consumer( byval param as any ptr )
declare sub producer( byval param as any ptr )

sub destroyState( byref state as PRODUCER_CONSUMER_STATE )
	if state.can_consume <> 0 then conddestroy state.can_consume
	if state.can_produce <> 0 then conddestroy state.can_produce
	if state.lock <> 0 then mutexdestroy state.lock
end sub

sub requestShutdown( byref state as PRODUCER_CONSUMER_STATE )
	mutexlock state.lock
	state.shutdown_requested = TRUE
	condbroadcast state.can_consume
	condbroadcast state.can_produce
	mutexunlock state.lock
end sub

dim state as PRODUCER_CONSUMER_STATE
dim as any ptr consumer_id, producer_id

state.lock = mutexcreate( )
if state.lock = 0 then
	print "Error creating the producer/consumer mutex."
	end 1
end if

state.can_produce = condcreate( )
if state.can_produce = 0 then
	destroyState state
	print "Error creating the producer condition."
	end 1
end if

state.can_consume = condcreate( )
if state.can_consume = 0 then
	destroyState state
	print "Error creating the consumer condition."
	end 1
end if

consumer_id = threadcreate( @consumer, @state )
if consumer_id = 0 then
	destroyState state
	print "Error creating the consumer thread."
	end 1
end if

producer_id = threadcreate( @producer, @state )
if producer_id = 0 then
	requestShutdown state
	threadwait consumer_id
	destroyState state
	print "Error creating the producer thread."
	end 1
end if

threadwait producer_id
threadwait consumer_id
destroyState state

sub consumer( byval param as any ptr )
	dim state as PRODUCER_CONSUMER_STATE ptr = _
	    cast(PRODUCER_CONSUMER_STATE ptr, param)
	if state = 0 then exit sub

	for i as integer = 0 to ITEM_COUNT - 1
		mutexlock state->lock

		while (state->item_ready = FALSE) andalso _
		      (state->shutdown_requested = FALSE)
			condwait state->can_consume, state->lock
		wend

		if state->shutdown_requested andalso (state->item_ready = FALSE) then
			mutexunlock state->lock
			exit sub
		end if

		print ", consumer gets: "; i
		state->item_ready = FALSE
		condsignal state->can_produce
		mutexunlock state->lock
	next
end sub

sub producer( byval param as any ptr )
	dim state as PRODUCER_CONSUMER_STATE ptr = _
	    cast(PRODUCER_CONSUMER_STATE ptr, param)
	if state = 0 then exit sub

	for i as integer = 0 to ITEM_COUNT - 1
		mutexlock state->lock

		while (state->item_ready <> FALSE) andalso _
		      (state->shutdown_requested = FALSE)
			condwait state->can_produce, state->lock
		wend

		if state->shutdown_requested then
			mutexunlock state->lock
			exit sub
		end if

		print "Producer puts: "; i;
		state->item_ready = TRUE
		condsignal state->can_consume
		mutexunlock state->lock
	next
end sub
