#include "icerlserial.h"

HANDLE* ports;
int port_cnt = -1; // port number before creating array of handles
int cur_port = 0;  // current opened ports index

static ERL_NIF_TERM init_ports(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
	int cnt = 0, ret = 0;
    if (!enif_get_int(env, argv[0], &cnt)) {
		return enif_make_badarg(env);
    }

	if(cnt > 255)
		return enif_make_badarg(env);

	port_cnt = cnt;
	cur_port = 0;

	ports = (HANDLE *) malloc(sizeof(HANDLE)*port_cnt);

	return enif_make_int(env, ret);
}

static ERL_NIF_TERM free_ports(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
	int ret = 0;
    
	if(port_cnt >= 0)
		free(ports);

	return enif_make_int(env, ret);
}

// start_port(_PORT, _BAUD, _DB, _PARITY, _SB)
static ERL_NIF_TERM start_port(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
	int ret = 0;

	DCB dcb;
	HANDLE hPort;

	char port_name[32];
	int baud;
	int db;
	int parity;
	int sb;

	wchar_t port_wname[32];

	if (cur_port + 1 >= port_cnt)
		return enif_make_string(env, "All ports are busy!", ERL_NIF_LATIN1);
	
	if (argc < 5)
		return enif_make_badarg(env);

	if (!enif_get_string (env, argv[0], port_name, sizeof(port_name), ERL_NIF_LATIN1))
		return enif_make_badarg(env);

	if (!enif_get_int (env, argv[1], &baud))
		return enif_make_badarg(env);

	if (!enif_get_int (env, argv[2], &db))
		return enif_make_badarg(env);

	if (!enif_get_int (env, argv[3], &parity))
		return enif_make_badarg(env);

	if (!enif_get_int (env, argv[4], &sb))
		return enif_make_badarg(env);

	swprintf(port_wname, sizeof(port_wname), L"%hs", port_name);

	hPort =  CreateFile( 
			port_wname, 
			GENERIC_READ | GENERIC_WRITE, 
			0, 
			NULL, 
			OPEN_EXISTING, 
			0, 
			NULL 
			); 

	if (!GetCommState(hPort,&dcb)) 
		return enif_make_string(env, "Port can't be opened!", ERL_NIF_LATIN1);

	dcb.BaudRate = (DWORD) baud; //  CBR_9600; //9600 Baud 
	dcb.ByteSize = db; //8 data bits 
	dcb.Parity = parity; // NOPARITY; //no parity 
	dcb.StopBits = sb; // ONESTOPBIT; //1 stop 

	if (!SetCommState(hPort,&dcb)) 
		return enif_make_string(env, "Port can't be configured!", ERL_NIF_LATIN1);

	ports[cur_port] = hPort;
	cur_port = cur_port + 1;

	return enif_make_int(env, cur_port);
}

static int data_ready_count(HANDLE port_device){
	DWORD dwErrorFlags;
	COMSTAT ComStat;

	ClearCommError( port_device, &dwErrorFlags, &ComStat );

	return ComStat.cbInQue;
}

static ERL_NIF_TERM data_ready(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
	int ret = 0;
	int port = 0;

	if (argc < 1)
		return enif_make_badarg(env);

	if (!enif_get_int (env, argv[0], &port))
		return enif_make_badarg(env);

	if (port > cur_port || port < 1)
		return enif_make_badarg(env);

	if(ports[port - 1] == NULL)
		return enif_make_string(env, "Port is already closed!", ERL_NIF_LATIN1);

	ret = data_ready_count(ports[port - 1]);
	
	return enif_make_int(env, ret);
}


// read_com(_PORTID, _DATACNT)
static ERL_NIF_TERM read_com(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
	int ret = 0;
	int port = 0;
	int data_cnt = 0;
	DWORD dwBytesTransferred;
	ErlNifBinary bin;
	unsigned char* data;

	if (argc < 2)
		return enif_make_badarg(env);

	if (!enif_get_int (env, argv[0], &port))
		return enif_make_badarg(env);

	if (!enif_get_int (env, argv[1], &data_cnt))
		return enif_make_badarg(env);

	if (port > cur_port || port < 1 || data_cnt < 1 || data_cnt > INT_MAX)
		return enif_make_badarg(env);

	if(ports[port - 1] == NULL)
		return enif_make_string(env, "Port is already closed!", ERL_NIF_LATIN1);

	data = (unsigned char*) malloc(sizeof(unsigned char)*(data_cnt));

	ReadFile (ports[port - 1], data, data_cnt, &dwBytesTransferred, 0); //read 1 

	if(! enif_alloc_binary(dwBytesTransferred, &bin)){
		return enif_make_string(env, "Memory cannot be allocated!", ERL_NIF_LATIN1);
	}

	memcpy(bin.data, data, dwBytesTransferred);

	return enif_make_binary(env, &bin);
}

static ERL_NIF_TERM write_com(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
	int ret = 0;
	int port = 0;
	DWORD dwBytesTransferred;
	ErlNifBinary bin;

	if (argc < 2)
		return enif_make_badarg(env);

	if (!enif_get_int (env, argv[0], &port))
		return enif_make_badarg(env);

	if (!enif_inspect_binary(env, argv[1], &bin))
		return enif_make_badarg(env);
    
	if (port > cur_port || port < 1 || bin.size < 1 || bin.size > INT_MAX)
		return enif_make_badarg(env);

	if(ports[port - 1] == NULL)
		return enif_make_string(env, "Port is already closed!", ERL_NIF_LATIN1);

	if((int) WriteFile(ports[port - 1], bin.data, (DWORD) bin.size, &dwBytesTransferred, NULL))
		ret = dwBytesTransferred;
	else
		ret = 0;

	return enif_make_int(env, ret);
}

static ERL_NIF_TERM set_rts(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
	int ret = 0;
	int port = 0;
	int state = 0;

	if (argc < 1)
		return enif_make_badarg(env);

	if (!enif_get_int (env, argv[0], &port))
		return enif_make_badarg(env);

	if (!enif_get_int (env, argv[0], &state))
		return enif_make_badarg(env);

	if (port > cur_port || port < 1)
		return enif_make_badarg(env);

	if(ports[port - 1] == NULL)
		return enif_make_string(env, "Port is already closed!", ERL_NIF_LATIN1);

	if(state)
		ret = (int) EscapeCommFunction(ports[port - 1], SETRTS);
	else
		ret = (int) EscapeCommFunction(ports[port - 1], CLRRTS);

	return enif_make_int(env, ret);
}

static ERL_NIF_TERM set_dtr(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
	int ret = 0;
	int port = 0;
	int state = 0;

	if (argc < 1)
		return enif_make_badarg(env);

	if (!enif_get_int (env, argv[0], &port))
		return enif_make_badarg(env);

	if (!enif_get_int (env, argv[0], &state))
		return enif_make_badarg(env);

	if (port > cur_port || port < 1)
		return enif_make_badarg(env);

	if(ports[port - 1] == NULL)
		return enif_make_string(env, "Port is already closed!", ERL_NIF_LATIN1);

	if(state)
		ret = (int) EscapeCommFunction(ports[port - 1], SETDTR);
	else
		ret = (int) EscapeCommFunction(ports[port - 1], CLRDTR);

	return enif_make_int(env, ret);
}

static ERL_NIF_TERM stop_port(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]){
	int ret = 0;
	int port = 0;

	if (argc < 1)
		return enif_make_badarg(env);

	if (!enif_get_int (env, argv[0], &port))
		return enif_make_badarg(env);

	if (port > cur_port || port < 1)
		return enif_make_badarg(env);

	if(ports[port - 1])
		CloseHandle(ports[port - 1]);

	ports[port - 1] = NULL;

	return enif_make_int(env, ret);
}

static ErlNifFunc nif_funcs[] = {
	{"init_ports", 1, init_ports},
	{"start_port", 5, start_port},
	{"data_ready", 1, data_ready},
	{"read_com", 2, read_com},
	{"write_com", 2, write_com},
	{"set_rts", 2, set_rts},
	{"set_dtr", 2, set_dtr},
	{"stop_port", 1, stop_port},
	{"free_ports", 0, free_ports}
};

ERL_NIF_INIT(icerlserial, nif_funcs, NULL, NULL, NULL, NULL)

