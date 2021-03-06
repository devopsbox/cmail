int smtp_greet(LOGGER log, CONNECTION* conn, MTA_SETTINGS settings){
	CONNDATA* conn_data = (CONNDATA*)conn->aux_data;

	//try esmtp
	client_send(log, conn, "EHLO %s\r\n", settings.helo_announce);

	//await answer
	switch(protocol_expect(log, conn, SMTP_220_TIMEOUT, 250)){
		case 0:
			logprintf(log, LOG_DEBUG, "Negotiated ESMTP\n");
			conn_data->extensions_supported = true;
			return 0;

		case -1:
			logprintf(log, LOG_ERROR, "Failed to read EHLO response\n");
			return -1;
		default:
			break;
	}

	logprintf(log, LOG_INFO, "EHLO failed with %d, trying HELO\n", conn_data->reply.code);
	client_send(log, conn, "HELO %s\r\n", settings.helo_announce);

	if(protocol_expect(log, conn, SMTP_220_TIMEOUT, 250)){ //FIXME the rfc does not define a timeout for this
		logprintf(log, LOG_WARNING, "Could not negotiate any protocol, HELO response was %d\n", conn_data->reply.code);
		return -1;
	}

	logprintf(log, LOG_INFO, "Negotiated SMTP\n");
	return 0;
}

#ifndef CMAIL_NO_TLS
int smtp_starttls(LOGGER log, CONNECTION* conn){
	CONNDATA* conn_data = (CONNDATA*)conn->aux_data;

	if(!conn_data->extensions_supported){
		logprintf(log, LOG_ERROR, "Extensions not supported, not negotiating TLS\n");
		return -1;
	}

	client_send(log, conn, "STARTTLS\r\n");

	if(protocol_expect(log, conn, SMTP_220_TIMEOUT, 220)){ //FIXME the rfc does not define a timeout for this
		logprintf(log, LOG_WARNING, "Could not start TLS negotiation, response was %d\n", conn_data->reply.code);
		return -1;
	}

	//perform handshake
	if(tls_handshake(log, conn) < 0){
		logprintf(log, LOG_ERROR, "Failed to negotiate TLS\n");
		return -1;
	}

	return 0;
}
#endif

int smtp_auth(LOGGER log, CONNECTION* conn, char* auth_data){
	CONNDATA* conn_data = (CONNDATA*)conn->aux_data;

	if(!conn_data->extensions_supported){
		logprintf(log, LOG_WARNING, "Remote authentication requested, but server does not support extensions\n");
		return -1;
	}

	client_send(log, conn, "AUTH PLAIN\r\n");

	if(protocol_expect(log, conn, SMTP_220_TIMEOUT, 334)){
		logprintf(log, LOG_WARNING, "Could not start remote authentication, response was %d\n", conn_data->reply.code);
		return -1;
	}

	//send authentication data
	client_send(log, conn, "%s\r\n", auth_data);
	
	if(protocol_expect(log, conn, SMTP_220_TIMEOUT, 235)){
		logprintf(log, LOG_WARNING, "Remote authentication failed with status %d\n", conn_data->reply.code);
		return -1;
	}

	logprintf(log, LOG_DEBUG, "Successfully authenticated with remote\n");
	return 0;
}

int smtp_initiate(LOGGER log, CONNECTION* conn, MAIL* mail){
	CONNDATA* conn_data = (CONNDATA*)conn->aux_data;
	//calling contract: 0 -> continue, 1 -> fail temp, -1 -> fail perm
	//need to accept NULL as sender here in order to handle bounces
	client_send(log, conn, "MAIL FROM:<%s>\r\n", mail->envelopefrom ? mail->envelopefrom:"");

	if(protocol_expect(log, conn, SMTP_MAIL_TIMEOUT, 250)){
		//handle protocol failures as temporary, only 500 as permanent
		return (conn_data->reply.code >= 500 && conn_data->reply.code <= 599) ? -1:1;
	}
	return 0;
}

int smtp_rcpt(LOGGER log, CONNECTION* conn, char* path){
	//Calling contract: retn 0 -> accepted, 1 -> fail temp, -1 -> fail perm
	CONNDATA* conn_data = (CONNDATA*)conn->aux_data;

	client_send(log, conn, "RCPT TO:<%s>\r\n", path);

	if(protocol_read(log, conn, SMTP_RCPT_TIMEOUT) < 0){
		logprintf(log, LOG_ERROR, "Failed to read response to recipient\n");
		return 1;
	}

	if(conn_data->reply.code == 250){
		return 0;
	}

	logprintf(log, LOG_WARNING, "Recipient response code %d\n", conn_data->reply.code);

	if(conn_data->reply.code >= 400 && conn_data->reply.code <= 499){
		return 1;
	}

	return -1;
}

int smtp_data(LOGGER log, CONNECTION* conn, char* mail_data){
	CONNDATA* conn_data = (CONNDATA*)conn->aux_data;
	char* mail_bytestuff;

	//Calling contract: retn 0 -> accepted, 1 -> fail temp, -1 -> fail perm
	client_send(log, conn, "DATA\r\n");

	if(protocol_expect(log, conn, SMTP_DATA_TIMEOUT, 354)){
		//FIXME might wanna test for other responses here
		logprintf(log, LOG_WARNING, "Data initiation failed, response was %d\n", conn_data->reply.code);
		return -1;
	}

	if(mail_data[0] == '.'){
		client_send(log, conn, ".");
	}
	do{
		mail_bytestuff = strstr(mail_data, "\r\n.");
		if(mail_bytestuff){
			client_send_raw(log, conn, mail_data, mail_bytestuff - mail_data);
			client_send(log, conn, "\r\n..");
			mail_data=mail_bytestuff + 3;
		}
		else{
			//logprintf(log, LOG_DEBUG, "Sending %d bytes message data\n", strlen(mail_data));
			client_send_raw(log, conn, mail_data, strlen(mail_data));
		}
	}
	while(mail_bytestuff);

	client_send(log, conn, "\r\n.\r\n");

	if(protocol_read(log, conn, SMTP_DATA_TERMINATION_TIMEOUT) < 0){
		logprintf(log, LOG_ERROR, "Failed to read data terminator response\n");
		return 1;
	}

	if(conn_data->reply.code == 250){
		logprintf(log, LOG_INFO, "Mail accepted\n");
		return 0;
	}

	logprintf(log, LOG_WARNING, "Data terminator response code %d\n", conn_data->reply.code);

	if(conn_data->reply.code >= 400 && conn_data->reply.code <= 499){
		return 1;
	}

	return -1;
}

int smtp_rset(LOGGER log, CONNECTION* conn){
	CONNDATA* conn_data = (CONNDATA*)conn->aux_data;
	client_send(log, conn, "RSET\r\n");

	if(protocol_expect(log, conn, SMTP_220_TIMEOUT, 250)){ //FIXME the rfc does not define a timeout for this
		logprintf(log, LOG_WARNING, "Could not reset connection, response was %d\n", conn_data->reply.code);
		return -1;
	}
	return 0;
}

int smtp_noop(LOGGER log, CONNECTION* conn){
	CONNDATA* conn_data = (CONNDATA*)conn->aux_data;
	client_send(log, conn, "NOOP\r\n");

	if(protocol_expect(log, conn, 20, 250)){ //FIXME the rfc does not define a timeout for this
		logprintf(log, LOG_WARNING, "Could not noop, response was %d\n", conn_data->reply.code);
		return -1;
	}
	return 0;
}

int smtp_negotiate(LOGGER log, MTA_SETTINGS settings, char* remote, CONNECTION* conn, REMOTE_PORT port, char* auth_data){
	unsigned u;

	//if(conn_data->state==STATE_NEW){
		//initialize first-time data
		#ifndef CMAIL_NO_TLS
		if(conn->tls_mode == TLS_NONE){
			tls_init_clientpeer(log, conn, remote, settings.tls_credentials);

			if(port.tls_mode == TLS_ONLY){
				//perform handshake immediately
				if(tls_handshake(log, conn) < 0){
					logprintf(log, LOG_ERROR, "Failed to negotiate TLS with TLSONLY remote\n");
					return -1;
				}
			}
		}
		#endif
	//}

	//await 220
	if(protocol_expect(log, conn, SMTP_220_TIMEOUT, 220)){
		logprintf(log, LOG_ERROR, "Initial SMTP response failed or not properly formatted\n");
		return -1;
	}

	//negotiate smtp
	if(smtp_greet(log, conn, settings) < 0){
		logprintf(log, LOG_WARNING, "Failed to negotiate SMTP/ESMTP\n");
		return -1;
	}

	#ifndef CMAIL_NO_TLS
	if(port.tls_mode == TLS_NEGOTIATE && conn->tls_mode == TLS_NONE){
		//if requested, proto_starttls
		if(smtp_starttls(log, conn) < 0){
			logprintf(log, LOG_ERROR, "Failed to negotiate TLS layer\n");
			return -1;
		}

		//perform greeting again
		if(smtp_greet(log, conn, settings) < 0){
			logprintf(log, LOG_WARNING, "Failed to negotiate SMTPS/ESMTPS\n");
			return -1;
		}
	}

	//do tls padding
	if(settings.tls_padding && conn->tls_mode == TLS_ONLY){
		if(common_rand(&u, sizeof(u)) < 0){
			u = settings.tls_padding;
		}
		else{
			u %= settings.tls_padding;
		}

		for(; u > 0; u--){
			if(u % 2){
				smtp_noop(log, conn);
			}
			else{
				smtp_rset(log, conn);
			}
		}
	}
	#endif

	//if requested, try to authenticate
	if(auth_data && smtp_auth(log, conn, auth_data) < 0){
		logprintf(log, LOG_WARNING, "Failed to authenticate with remote host\n");
		return -1;
	}

	return 0;
}
