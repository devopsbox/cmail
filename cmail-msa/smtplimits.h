#define SMTP_MAX_LINE_LENGTH 		1500	//RFC 5321 4.5.3.1.6: Minimum 1000 Bytes
#define CMAIL_RECEIVE_BUFFER_LENGTH	2048
#define SMTP_MAX_RECIPIENTS		150	//RFC 5321 4.5.3.1.8: Minimum 100 Recipients
#define SMTP_MAX_PATH_LENGTH		300	//RFC 5321 4.5.3.1.3: Maximum 256 Bytes
#define CMAIL_MAX_POOLED_PATHS		500	//Global maximum of path objects at any time, recommended per RFC: SMTP_MAX_RECIPIENTS*MAX_SIMULTANEOUS_CLIENTS
#define CMAIL_MAX_CONCURRENT_CLIENTS 	64
#define CMAIL_MESSAGEID_MAX		70	//For POP3 UIDL
#define MAX_FQDN_LENGTH			300 	//actually, 255
#define SMTP_HEADER_LINE_MAX		80
#define SMTP_SERVER_TIMEOUT		300	//RFC 5321 4.5.3.2.7: 5 Minutes
