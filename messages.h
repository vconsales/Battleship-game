typedef enum 
{ 
	WELCOME_MESS,
	PEER_SETS_NAME,
	PEER_SETS_UDP_PORT,
	NAME_ACCEPTED,
	NAME_REFUSED,
	REGISTERED,
	LIST_OF_PEERS,
	REQ_CONN_TO_PEER,
	PEER_DOESNT_EXIST,
	ACCEPT_CONN,
	GENERIC_ERR,
	DISCONNECT,
} message_type;

#define INIT_REG_SET_NAME { PEER_SETS_NAME, {"\0"} }
#define INIT_REG_SET_UDP_PORT { PEER_SETS_UDP_PORT, 0 }
#define INIT_ACCEPT_CONN_TO_PEER { ACCEPT_CONN, -1, -1 }

typedef struct reg_set_name_t
{
	message_type t;
	char name[65];
}__attribute__((packed)) reg_set_name;

typedef struct reg_set_udp_port_t
{
	message_type t;
	uint16_t udp_port;
}__attribute__((packed))reg_set_udp_port;

typedef struct req_conn_to_peer_t
{
	message_type t;
	int sender_id;
	char sender_name[65];
}__attribute__((packed)) req_conn_to_peer;

typedef struct accept_conn_to_peer_t
{
	message_type t;
	int sender_id;
	int receiver_id;
}__attribute__((packed)) accept_conn_to_peer;