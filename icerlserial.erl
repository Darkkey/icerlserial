-module(icerlserial).
-export([start_port/5, stop_port/1, read_com/2, uread_com/1, data_ready/1, write_com/2, set_rts/2, set_dtr/2, free_ports/0]).
-on_load(init/0).

init() ->
    ok = erlang:load_nif("x64/Debug/icerlserial", 0),
	init_ports(32),
	ok.

init_ports(_X) ->
    exit(nif_library_not_loaded).

start_port(_PORT, _BAUD, _DB, _PARITY, _SB) ->
    exit(nif_library_not_loaded).

stop_port(_PORTID) ->
    exit(nif_library_not_loaded).

uread_com(PortID) ->
	Cnt = data_ready(PortID),
	if Cnt > 0 -> read_com(PortID, Cnt);
	   true -> <<>>
	end.

read_com(PORTID, _BINCNT) ->
	exit(nif_library_not_loaded).

data_ready(_PORTID) ->
    exit(nif_library_not_loaded).

write_com(_PORTID, _DATA) ->
    exit(nif_library_not_loaded).

set_rts(_PORTID, _STATE) ->
    exit(nif_library_not_loaded).

set_dtr(_PORTID, _STATE) ->
    exit(nif_library_not_loaded).

free_ports() ->
    exit(nif_library_not_loaded).


