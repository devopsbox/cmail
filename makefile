PREFIX?=/usr/sbin
LOGDIR?=/var/log/cmail
CONFDIR?=/etc/cmail
DBDIR?=$(CONFDIR)/databases
.PHONY: clean install

all:
	@$(MAKE) -C cmail-msa
	@$(MAKE) -C cmail-mta
	@$(MAKE) -C cmail-popd
	@$(MAKE) -C cmail-imapd
	@$(MAKE) -C cmail-admin

	@-mkdir -p bin
	@mv cmail-msa/cmail-msa bin/
	@mv cmail-mta/cmail-mta bin/
	@mv cmail-popd/cmail-popd bin/
	# mv cmail-imapd/cmail-imapd bin/

install:
	@printf "Installing to %s\n" "$(PREFIX)"
	install -m 0755 bin/cmail-msa "$(PREFIX)"
	install -m 0755 bin/cmail-mta "$(PREFIX)"
	install -m 0755 bin/cmail-popd "$(PREFIX)"
	#install -m 0755 bin/cmail-imapd "$(PREFIX)"

init:
	@printf "Creating cmail user\n"
	adduser --disabled-login cmail
	@printf "Creating configuration directories\n"
	mkdir -p "$(LOGDIR)"
	mkdir -p "$(CONFDIR)"
	mkdir -p "$(DBDIR)"
	chown root:cmail "$(DBDIR)"
	chmod 770 "$(DBDIR)"
	@printf "Copying example configuration files to %s\n" "$(CONFDIR)"
	cp example-configs/* "$(CONFDIR)"
	@printf "Creating empty master database in %s/master.db3\n" "$(DBPATH)"
	cat sql-update/install_master.sql | sqlite3 "$(DBPATH)/master.db3"
	chmod 770 "$(DBPATH)/master.db3"

tls-init: init
	printf "Creating certificate storage directory in %s/keys" "$(CONFDIR)"
	mkdir -p "$(CONFDIR)/keys"
	chmod 700 "$(CONFDIR)/keys"
	printf "Creating temporary TLS certificate in %s/keys\n" "$(CONFDIR)"
	openssl req -x509 -newkey rsa:8192 -keyout "$(CONFDIR)/keys/temp.key" -out "$(CONFDIR)/keys/temp.cert" -days 100 -nodes
	chmod 600 "$(CONFDIR)/keys/*"

clean:
	rm bin/*


