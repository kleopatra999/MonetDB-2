/*
 * The contents of this file are subject to the MonetDB Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://monetdb.cwi.nl/Legal/MonetDBLicense-1.1.html
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the MonetDB Database System.
 *
 * The Initial Developer of the Original Code is CWI.
 * Portions created by CWI are Copyright (C) 1997-July 2008 CWI.
 * Copyright August 2008-2011 MonetDB B.V.
 * All Rights Reserved.
 */


#include "sql_config.h"
#include "store_connections.h"



/*Function to create a connection*/
int
sql_trans_connect_catalog(sql_trans* tr, char *server, int port, char *db, char *db_alias, char *user, char *passwd, char *lang)
{
	int id = store_next_oid(), port_l = port;
	sql_schema * s = find_sql_schema(tr, "sys");
	sql_table *t = find_sql_table(s, "connections");
	sql_column *c_server = find_sql_column(t, "server");
	sql_column *c_db = find_sql_column(t, "db");
	sql_column *c_db_alias = find_sql_column(t, "db_alias");

	if ((table_funcs.column_find_row(tr, c_server, server, c_db, db, NULL) == oid_nil) && (table_funcs.column_find_row(tr, c_db_alias, db_alias, NULL) == oid_nil)) {
		table_funcs.table_insert(tr, t, &id, server, &port_l, db, db_alias, user, passwd, lang);
		return id;
	}
	
	return 0;
}

/*Function to drop the connection*/
int
sql_trans_disconnect_catalog(sql_trans* tr, char * db_alias)
{
	oid rid = oid_nil;
	int id = 0;
	sql_schema * s = find_sql_schema(tr, "sys");
	sql_table* t = find_sql_table(s, "connections");

	sql_column * col_db_alias = find_sql_column(t, "db_alias");
	sql_column * col_id = find_sql_column(t, "id");

	rid = table_funcs.column_find_row(tr, col_db_alias, db_alias, NULL);
	if (rid != oid_nil) {
		id = *(int *) table_funcs.column_find_value(tr, col_id, rid);
		table_funcs.table_delete(tr, t, rid);
	} else {
		id = 0;
	}
	return id;
}

int
sql_trans_disconnect_catalog_ALL(sql_trans* tr)
{
	sql_schema * s = find_sql_schema(tr, "sys");
	sql_table* t = find_sql_table(s, "connections");

	sql_trans_clear_table(tr, t);
	return 1;
}

list*
sql_trans_get_connection(sql_trans* tr, int id, char *server, char *db, char *db_alias, char *user)
{
	oid rid = oid_nil;
	sql_schema *s = find_sql_schema(tr, "sys");	
	sql_table *con = find_sql_table(s, "connections");
	list *con_arg_list = list_create((fdestroy) NULL); 
	sql_column *col_id, *col_server, *col_db, *col_db_alias, *col_user, *col_port, *col_passwd, *col_lang;
	char dbalias[BUFSIZ];

	col_id = find_sql_column(con, "id");
	col_db_alias = find_sql_column(con, "db_alias");
	col_server = find_sql_column(con, "server");
	col_db = find_sql_column(con, "db");
	col_user = find_sql_column(con, "user");
	col_port = find_sql_column(con, "port");
	col_passwd = find_sql_column(con, "password");
	col_lang = find_sql_column(con, "language");
	
	if (db_alias == NULL){
		snprintf(dbalias, BUFSIZ, "%s_%s_%s", server, db, user);
		db_alias = dbalias;
	}

	if (id != -1)
		rid = table_funcs.column_find_row(tr, col_id, &id, NULL);
	else 	
		rid = table_funcs.column_find_row(tr, col_db_alias, db_alias, NULL);

	if (rid != oid_nil) {
		list_append(con_arg_list, (int *) table_funcs.column_find_value(tr, col_id, rid));
		list_append(con_arg_list, (char *) table_funcs.column_find_value(tr, col_server, rid));
		list_append(con_arg_list, (int *) table_funcs.column_find_value(tr, col_port, rid));
		list_append(con_arg_list, (char *) table_funcs.column_find_value(tr, col_db, rid));
		list_append(con_arg_list, (char *) table_funcs.column_find_value(tr, col_db_alias, rid));
		list_append(con_arg_list, (char *) table_funcs.column_find_value(tr, col_user, rid));
		list_append(con_arg_list, (char *) table_funcs.column_find_value(tr, col_passwd, rid));
		list_append(con_arg_list, (char *) table_funcs.column_find_value(tr, col_lang, rid));
	}

	return con_arg_list;
}
