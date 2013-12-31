<?
/* Cherokee's DBSlayer Wrapper for MySQL built-in functions
 *
 * Authors:
 *      Taher Shihadeh <taher@unixwars.com>
 *
 * Copyright (C) 2001-2014 Alvaro Lopez Ortega <alvaro@alobbs.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * The name "Alvaro Lopez Ortega" may not be used to endorse or
 *   promote products derived from this software without specific prior
 *   written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* NOTE:
 *
 * To override the mysql_* builtin functions, you need to provide
 * phpize (php5-dev) and: `pecl install apd`.
 *
 * If you do not, you can still use the wrapper initializing it with a
 * FALSE value as optional second parameter: cherokee_init($host, FALSE)
 *
 * runkit_function_redefine could be another approach. Remains to be seen.
 *
 */

define("DEFAULT_ERRNO", 3000);
define("DEFAULT_ERROR", "DBSlayer: undefined MySQL error");
define("SERVER_INFO",   "Cherokee DBSlayer MySQL bridge");
define("CLIENT_INFO",   "DBSlayer MySQL Wrapper v1.0");
define("DEFAULT_LINK",  "DBSLAYER_LINK");
define("DEFAULT_HOST",  "http://localhost:8888");

$_dbslayer = NULL;

function cherokee_init ($host, $map = TRUE)
{
	global $_dbslayer;

	$curl = curl_init();
	if (!$curl)
	   throw new Exception("DBSlayer MySQL Wrapper could not be created.");

	$_dbslayer = array (
	       'db_host' => ($host) ? $host : ini_get('mysql.default_host'),
	       'db_user' => ini_get('mysql.default_user'),
	       'db_pass' => ini_get('mysql.default_password'),
	       'db_link' => DEFAULT_LINK,
	       'db_name' => NULL,
	       'encoding'=> ini_get('mysql.character_set'),
	       'curl'    => $curl,
	       'success' => NULL,
	       'errno'   => NULL,
	       'error'   => NULL,
	       'insert_id'=> 0,
	       'affected_rows'=> 0);

	if ($map == TRUE)
	   return _dbslayer_map();
	return TRUE;
}

/* Debug */
function _dbslayer_print()
{
	global $_dbslayer;
        echo 'db_host: '. $_dbslayer['db_host']."\n";
    	echo 'db_user: '. $_dbslayer['db_user']."\n";
    	echo 'db_pass: '. $_dbslayer['db_pass']."\n";
    	echo 'db_link: '. $_dbslayer['db_link']."\n";
    	echo 'db_name: '. $_dbslayer['db_name']."\n";
    	echo 'success: '. $_dbslayer['success']."\n";
    	echo 'errno: '. $_dbslayer['errno']."\n";
    	echo 'error: '. $_dbslayer['error']."\n";
    	echo 'insert_id: '. $_dbslayer['insert_id']."\n";
    	echo 'affected_rows: '. $_dbslayer['affected_rows']."\n";
}

/* Transparently use DBSLAYER when approriate
 */
function _dbslayer_map()
{
	$substs = array(
	    'is_resource'		=> 'cherokee_is_resource($link)',
	    'mysql_affected_rows'	=> 'cherokee_mysql_affected_rows($link)',
	    'mysql_client_encoding'	=> 'cherokee_mysql_client_encoding ($link)',
	    'mysql_close'		=> 'cherokee_mysql_close($link)',
	    'mysql_connect' 	   	=> 'cherokee_mysql_connect($host, $user, $pass, $new_link, $client_flags)',
	    'mysql_create_db'		=> 'cherokee_mysql_create_db($database_name, $link)',
	    'mysql_data_seek' 		=> 'cherokee_mysql_data_seek($result, $row_number)',
	    'mysql_db_name'		=> 'cherokee_mysql_db_name($result, $row, $field)',
	    'mysql_db_query'		=> 'cherokee_mysql_db_query($database, $query, $link)',
	    'mysql_drop_db'		=> 'cherokee_mysql_drop_db($database_name, $link)',
	    'mysql_errno' 		=> 'cherokee_mysql_errno($link)',
	    'mysql_error' 		=> 'cherokee_mysql_error($link)',
	    'mysql_escape_string'	=> 'cherokee_mysql_escape_string($unescaped_string)',
	    'mysql_fetch_array' 	=> 'cherokee_mysql_fetch_array($result, $result_type)',
	    'mysql_fetch_assoc' 	=> 'cherokee_mysql_fetch_assoc($result)',
	    'mysql_fetch_field' 	=> 'cherokee_mysql_fetch_field($result, $field_offset)',
	    'mysql_fetch_lengths'	=> 'cherokee_mysql_fetch_lengths($result)',
	    'mysql_fetch_num'		=> 'cherokee_mysql_fetch_num($result)',
	    'mysql_fetch_object'	=> 'cherokee_mysql_fetch_object (&$result, $class_name, $params)',
	    'mysql_field_flags'		=> 'cherokee_mysql_field_flags ($result, $field_offset)',
	    'mysql_field_name'		=> 'cherokee_mysql_field_name ($result, $field_offset)',
	    'mysql_field_seek'		=> 'cherokee_mysql_field_seek (&$result, $field_offset)',
	    'mysql_field_type'		=> 'cherokee_mysql_field_type ($result, $field_offset)', 
	    'mysql_free_result' 	=> 'cherokee_mysql_free_result($result)',
	    'mysql_get_client_info'   	=> 'cherokee_mysql_get_client_info()',
	    'mysql_get_host_info'	=> 'cherokee_mysql_get_host_info ($link)',
	    'mysql_get_proto_info'	=> 'cherokee_mysql_get_proto_info ($link)',
	    'mysql_get_server_info'   	=> 'cherokee_mysql_get_server_info($link)',
	    'mysql_info'		=> 'cherokee_mysql_info ($link)',
	    'mysql_insert_id'    	=> 'cherokee_mysql_insert_id($link)',
	    'mysql_list_dbs' 		=> 'cherokee_mysql_list_dbs($link)',
	    'mysql_list_fields'		=> 'cherokee_mysql_list_fields ($database, $table_name, $link)',
	    'mysql_list_processes'	=> 'cherokee_mysql_list_processes ($link)',
	    'mysql_list_tables'		=> 'cherokee_mysql_list_tables  ($database, $link)',
	    'mysql_num_fields'		=> 'cherokee_mysql_num_fields ($result)',
	    'mysql_num_rows' 	   	=> 'cherokee_mysql_num_rows($result)',
	    'mysql_pconnect' 	   	=> 'cherokee_mysql_pconnect($host, $user, $pass, $client_flags)',
	    'mysql_ping'		=> 'cherokee_mysql_ping ($link)',
	    'mysql_query'	        => 'cherokee_mysql_query($query, $link)',
	    'mysql_real_escape_string'	=> 'cherokee_mysql_real_escape_string($unescaped_string, $link)',
	    'mysql_result' 		=> 'cherokee_mysql_result($result, $row, $field)',
	    'mysql_select_db'		=> 'cherokee_mysql_select_db($database_name, &$link)',
	    'mysql_set_charset'		=> 'cherokee_mysql_set_charset($charset, $link)',
	    'mysql_stat'		=> 'cherokee_mysql_stat($link)',
	    'mysql_tablename' 		=> 'cherokee_mysql_tablename($result, $i)',
	    'mysql_thread_id'		=> 'cherokee_mysql_thread_id ($link)',
	    'mysql_unbuffered_query'	=> 'cherokee_mysql_unbuffered_query($query, $link)',
    	    );

	$args = array(
    	    'is_resource'		=> '$link',
	    'mysql_affected_rows'	=> '$link = NULL',
	    'mysql_client_encoding'	=> '$link = NULL',
    	    'mysql_close'		=> '$link = NULL',
	    'mysql_connect' 	   	=> '$host = NULL, $user = NULL, $pass = NULL, $new_link = false, $client_flags = 0',
	    'mysql_create_db'		=> '$database_name, $link = NULL',
	    'mysql_data_seek' 		=> '&$result, $row_number',
	    'mysql_db_name'		=> '$result, $row, $field = NULL',
	    'mysql_db_query'		=> '$database, $query, $link = NULL',
	    'mysql_drop_db'		=> '$database_name, $link = NULL',
	    'mysql_errno' 		=> '$link = NULL',
	    'mysql_error' 		=> '$link = NULL',
	    'mysql_escape_string'	=> '$unescaped_string',
	    'mysql_fetch_array' 	=> '&$result, $result_type = MYSQL_BOTH',
	    'mysql_fetch_assoc' 	=> '&$result',
	    'mysql_fetch_field' 	=> '&$result, $field_offset = -1',
	    'mysql_fetch_lengths'	=> '$result',
	    'mysql_fetch_num'		=> '&$result',
	    'mysql_fetch_object'	=> '&$result, $class_name = "stdClass", $params = NULL',
	    'mysql_field_flags'		=> '$result, $field_offset',
	    'mysql_field_name'		=> '$result, $field_offset',
	    'mysql_field_seek'		=> '&$result, $field_offset',
	    'mysql_field_type'		=> '$result, $field_offset',
	    'mysql_free_result' 	=> '&$result',
	    'mysql_get_client_info'   	=> '',
	    'mysql_get_host_info'	=> '$link = NULL',
	    'mysql_get_proto_info'	=> '$link = NULL',
	    'mysql_get_server_info'   	=> '$link = NULL',
	    'mysql_info'		=> '$link = NULL',
	    'mysql_insert_id'    	=> '$link = NULL',
	    'mysql_list_dbs'		=> '$link = NULL',
	    'mysql_list_fields'		=> '$database, $table_name, $link = NULL',
	    'mysql_list_processes'	=> '$link = NULL',
	    'mysql_list_tables'		=> '$database, $link = NULL',
	    'mysql_num_fields'		=> '$result',
	    'mysql_num_rows' 	   	=> '$result',
	    'mysql_pconnect' 	   	=> '$host = NULL, $user = NULL, $pass = NULL, $client_flags = 0',
	    'mysql_ping'		=> '$link = NULL',
	    'mysql_query'	        => '$query, $link = NULL',
	    'mysql_real_escape_string'	=> '$unescaped_string, $link = NULL',
	    'mysql_result' 		=> '&$result, $row, $field = 0',
	    'mysql_select_db'		=> '$database_name, $link = NULL',
	    'mysql_set_charset'		=> '$charset , $link = NULL',
	    'mysql_stat'		=> '$link = NULL',
	    'mysql_tablename'		=> '$result, $i',
	    'mysql_thread_id'		=> '$link = NULL',
	    'mysql_unbuffered_query'	=> '$query, $link = NULL',
    	    );

	foreach ($substs as $func => $ren_func) {
	    $ret  = override_function($func, $args[$func], "return $substs[$func];");
      	    $ret &= rename_function("__overridden__", "_$ren_func");
	    if ($ret == FALSE) return FALSE;
  	}

	return TRUE;
}


/* Wrap DEFAULT_LINK checks and assignations */
function _dbslayer(&$link)
{
	global $_dbslayer;
	if ($link == NULL)
	   $link = $_dbslayer;

	if (isset($link['db_link']) && $link['db_link'] == DEFAULT_LINK)
	   return TRUE;
	return FALSE;
}


/* Delete NULL parameters before calling the wrapped functions */
function _call()
{
	$args_in  = func_get_args();
	$args_num = func_num_args();
	$args_out = array();
	$func	  = $args_in[0];

	for ($i=1; $i < $args_num ; $i++) {
	    if ($args_in[$i])
	       array_push($args_out, $args_in[$i]);
	}
	return call_user_func_array($func, $args_out);
}


/* Check if it is a DBSlayer result */
function _is_result($result)
{
	if (isset($result["_dbslayer"]))
	   return TRUE;
	return FALSE;
}


/* Fetch the RESULT entry */
function & _get_result (&$result)
{
	if (!_is_result($result))
	   return NULL;

	$idx = $result["_dbslayer"]["_idxres"];
	$tmp = &$result[$idx]["RESULT"];
	return $tmp;
}

/* Fetch the ROWS entry */
function & _get_rows (&$result)
{
	if (!_is_result($result))
	   return NULL;

	$idx = $result["_dbslayer"]["_idxres"];
	$tmp = &$result[$idx]["RESULT"]["ROWS"];
	return $tmp;
}


/* Fetch the HEADER entry */
function & _get_header (&$result)
{
	if (!_is_result($result))
	   return NULL;

	$idx = $result["_dbslayer"]["_idxres"];
	$tmp = &$result[$idx]["RESULT"]["HEADER"];
	return $tmp;
}

/* Fetch the [_dbslayer] private parameters */
function & _get_params (&$result)
{
	if (!_is_result($result))
	   return NULL;

	$tmp  = &$result["_dbslayer"];
	return $tmp;
}


/**********************************************************************
 * Wrapped functions from now on
 **********************************************************************/


/* is_resource - Finds whether a variable is a resource */
function cherokee_is_resource ($link)
{
	if (_dbslayer($link))
	   return TRUE;

	return _is_resource($link);
}


/* mysql_affected_rows - Get number of affected rows in previous MySQL operation */
function cherokee_mysql_affected_rows ($link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_affected_rows', $link);
	}

	return $link['affected_rows'];
}


/* mysql_client_encoding - Returns the name of the character set */
function cherokee_mysql_client_encoding ($link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_client_encoding', $link);
	}

	$query  = "SHOW VARIABLES WHERE Variable_name='character_set_connection';";
	$result = cherokee_mysql_query($query, $link);
	$result = cherokee_mysql_fetch_array($result);
	return $result['Value'];
}


/* mysql_close - Close MySQL connection */
function cherokee_mysql_close ($link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_close', $link);
	}

	return TRUE;
}


/* mysql_connect - Open a connection to a MySQL Server */
function cherokee_mysql_connect ($host = NULL, $user = NULL,
	     		   	 $pass = NULL, $new_link = false, 
			   	 $client_flags = 0)
{
	global $_dbslayer;
	$host = ($host) ? $host:$_dbslayer['db_host'];

	if ($host != $_dbslayer['db_host'])
	   return _mysql_connect($host, $user, $pass, $new_link, $client_flags);

	return cherokee_mysql_pconnect ($host, $user, $pass, $client_flags);
}


/* mysql_create_db - Create a MySQL database */
function cherokee_mysql_create_db ($database_name, $link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_create_db', $database_name, $link);
	}

	$query = "CREATE DATABASE $database_name";
	$result = cherokee_mysql_query($query, $link);
	return $result;
}


/* mysql_data_seek - Move internal result pointer */
function cherokee_mysql_data_seek  (&$result, $row_number)
{
	if (!_is_result($result))
	   return _mysql_data_seek ($result, $row_number);

	if ($row_number >= $result["_dbslayer"]["_rows"])
	   return FALSE;

	$result["_dbslayer"]["_idxrow"] = $row_number;
	return TRUE;
}


/* mysql_db_query - Send a MySQL query (and select a database). Deprecated */
function cherokee_mysql_db_query ($database, $query, $link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_db_query', $database, $query, $link);
	}

	cherokee_mysql_select_db($database);
	return cherokee_mysql_query($query, $link);
}


/* mysql_db_name - Get result data */
/* Deprecated alias: mysql_dbname */
function cherokee_mysql_db_name ($result, $row, $field = NULL)
{
	if (!_is_result($result))
	   return _mysql_db_name ($result, $row, $field);

	// I have no idea what $field is used for

	$tmp = _get_rows($result);

	if (isset($tmp[$row][0]))
	   return $tmp[$row][0];
	return FALSE;	
}


/* mysql_drop_db - Drop (delete) a MySQL database */
/* Deprecated alias: mysql_dropdb */
function cherokee_mysql_drop_db ($database_name, $link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_drop_db', $database_name, $link);
	}

	$query = "DROP  $database_name";
	$result = cherokee_mysql_query($query, $link);
	return $result;
}


/* mysql_errno - Returns the numerical value of the error message from previous MySQL operation */
function cherokee_mysql_errno ($link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_errno', $link);
	}

	return $link['errno'];
}	


/* mysql_error - Returns the text of the error message from previous MySQL operation */
function cherokee_mysql_error ($link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_error', $link);
	}

	return $link['error'];
}


/* mysql_fetch_array - Fetch a result row as an associative array, a numeric array, or both */
function cherokee_mysql_fetch_array  (&$result, $result_type = MYSQL_BOTH)
{
	if (!_is_result($result))
	   return _mysql_fetch_array (&$result, $result_type);

	// No RESULT field present
	if ($result["_dbslayer"]["_idxres"] < 0)
	   return FALSE;

	$idx = $result["_dbslayer"]["_idxres"];
	$tmp = &$result[$idx];
	// reset internal index
	if ($result["_dbslayer"]["_idxrow"] >= $result["_dbslayer"]["_rows"]) { 
	   $result["_dbslayer"]["_last_access"] = -1;
	   $result["_dbslayer"]["_idxrow"] = 0;
	   return FALSE;
	}

	$idx = $result["_dbslayer"]["_idxrow"];
	$result["_dbslayer"]["_last_access"] = $result["_dbslayer"]["_idxrow"];
	$result["_dbslayer"]["_idxrow"]++;

	$num = $tmp["RESULT"]["ROWS"][$idx];
	if ($result_type == MYSQL_NUM) {
	   return $num;
	}

	$keys = $tmp["RESULT"]["HEADER"];
	$vals = $tmp["RESULT"]["ROWS"][$idx];

	$assoc = array_combine($keys, $vals);
	if ($result_type == MYSQL_ASSOC) {
	   return $assoc;
	}
	return array_merge($num, $assoc);
}


/* mysql_fetch_assoc - Fetch a result row as an associative array */
function cherokee_mysql_fetch_assoc (&$result)
{
	return cherokee_mysql_fetch_array(&$result, MYSQL_ASSOC);
}


/* PARTIAL: mysql_fetch_field - Get column information from a result and return as an object */
function cherokee_mysql_fetch_field (&$result, $field_offset = -1)
{
	if (!_is_result($result)) {
	   if ($field_offset == -1)
	      return _mysql_fetch_field ($result);
	   return _mysql_fetch_field ($result, $field_offset);
	}

	if ($field_offset == -1) {
	   $field_offset = $result["_dbslayer"][_field_offset]+1;
	}

	$obj = _get_result($result);
	$fo = $field_offset;
	if ($fo >= count($obj["HEADER"])) {
	   $result["_dbslayer"][_field_offset] = 0;
	   return NULL;
	}

	$meta = new Mysql_fetch_field_object();
	$meta->name          = $obj["HEADER"][$fo];
	$meta->type          = $obj["TYPES"][$fo];
	$meta->blob          = 0; // 1 if the column is a BLOB
	$meta->numeric       = 0; // 1 if the column is numeric

	/* Many $meta-> fields remain UNIMPLEMENTED
	$meta->table         = $obj[""][$fo]; // name of the table the column belongs to
	$meta->def           = $obj[""][$fo]; // default value of the column
	$meta->max_length    = $obj[""][$fo]; // maximum length of the column
	$meta->not_null      = $obj[""][$fo]; // 1 if the column cannot be NULL
	$meta->primary_key   = $obj[""][$fo]; // 1 if the column is a primary key
	$meta->unique_key    = $obj[""][$fo]; // 1 if the column is a unique key
	$meta->multiple_key  = $obj[""][$fo]; // 1 if the column is a non-unique key
	$meta->unsigned      = $obj[""][$fo]; // 1 if the column is unsigned
	$meta->zerofill      = $obj[""][$fo]; // 1 if the column is zero-filled
	*/

	switch ($obj["TYPES"][$fo]) {
	       case MYSQL_TYPE_BLOB:
	       case MYSQL_TYPE_TINY_BLOB:
	       case MYSQL_TYPE_MEDIUM_BLOB:
	       case MYSQL_TYPE_LONG_BLOB:				
	       	    $meta->blob = 1;
		    break;

	       case MYSQL_TYPE_TINY:
	       case MYSQL_TYPE_SHORT:
	       case MYSQL_TYPE_LONG:
	       case MYSQL_TYPE_INT24:
	       case MYSQL_TYPE_DECIMAL:
	       case MYSQL_TYPE_NEWDECIMAL:
	       case MYSQL_TYPE_DOUBLE:
	       case MYSQL_TYPE_FLOAT:
	       case MYSQL_TYPE_LONGLONG:
	       	    $meta->numeric = 1;

	}
	$result["_dbslayer"][_field_offset]++;
	return $meta;
}


/* mysql_fetch_lengths - Get the length of each output in a result */
function cherokee_mysql_fetch_lengths ($result)
{
	if (!_is_result($result))
	   return _mysql_fetch_lengths ($result);

	$idx_last = $result["_dbslayer"]["_last_access"];
	if ($idx_last <0)
	   return FALSE;

	$rows    = _get_rows($result);
	$num     = $rows[$idx_last];
	$lengths = array();

	foreach ($num as $field) {
	   array_push ($lengths, strlen($field));
	}
	return $lengths;
}


/* mysql_fetch_object - Fetch a result row as an object */
function cherokee_mysql_fetch_object (&$result, $class_name = 'stdClass', $params = NULL)
{
	if (!_is_result($result)) {
	   return _call('_mysql_fetch_object', $result, $class_name, $params);
	}

	$obj = array();
	$row = cherokee_mysql_fetch_assoc (&$result);
	if (!$row) 
	   return FALSE;

	foreach ($row as $key=>$value) {
		$obj[$key] = $value;
	}

	$obj = (object) $obj;
	return $obj;
}


/* mysql_fetch_row - Get a result row as an enumerated array */
function cherokee_mysql_fetch_row (&$result)
{
	return cherokee_mysql_fetch_array(&$result, MYSQL_NUM);
}


/* PARTIAL: mysql_field_flags - Get the flags associated with the specified field in a result */
/* Deprecated alias: mysql_fieldflags */
function cherokee_mysql_field_flags ($result, $field_offset)
{
	if (!_is_result($result)) {
	   return _call('_mysql_field_flags', $result, $field_offset);
	}

	$field_num = count(_get_header($result));
	if ($field_offset < 0 || $field_offset >= $field_num) {
	   trigger_error('field_offset does not exist', E_WARNING);
	   return FALSE;
	}

	// The rest of the functionality cannot currently be
	// implemented. So fail:
	throw new Exception("cherokee_mysql_field_flags not functional!");
}


/* UNIMPLEMENTED: mysql_field_len - Returns the length of the specified field */
function cherokee_mysql_field_len ($result, $field_offset) {}


/* mysql_field_name - Get the name of the specified field in a result */
/* Deprecated alias: mysql_fieldname */
function cherokee_mysql_field_name ($result, $field_offset) {
	if (!_is_result($result)) {
	   return _call('_mysql_field_name', $result, $field_offset);
	}

	$header = _get_header($result);
	$field_num = count($header);
	if ($field_offset < 0 || $field_offset >= $field_num) {
	   trigger_error('field_offset does not exist', E_WARNING);
	   return FALSE;
	}

	return $header[$field_offset];
}


/* mysql_field_seek - Set result pointer to a specified field offset */
function cherokee_mysql_field_seek (&$result, $field_offset)
{
	if (!_is_result($result)) {
	   return _call('_mysql_field_seek', $result, $field_offset);
	}

	$params = _get_params($result);
	$field_num = $params["_fields"];

	if ($field_offset < 0 || $field_offset >= $field_num) {
	   trigger_error('field_offset does not exist', E_WARNING);
	   return FALSE;
	}

	$result['_dbslayer']['_field_offset'] = $field_offset;
	return TRUE;
}


/* UNIMPLEMENTED: mysql_field_table - Get name of the table the specified field is in */
function cherokee_mysql_field_table ($result, $field_offset) {}


/* mysql_field_type - Get the type of the specified field in a result */
function cherokee_mysql_field_type ($result, $field_offset) 
{
	if (!_is_result($result)) {
	   return _call('_mysql_field_type', $result, $field_offset);
	}

	$res = _get_result($result);
	$tmp = $res['TYPES'];
	$field_num = count($tmp);
	if ($field_offset < 0 || $field_offset >= $field_num) {
	   trigger_error('field_offset does not exist', E_WARNING);
	   return FALSE;
	}

	return $tmp[$field_offset];
}


/* mysql_free_result - Free result memory */
function cherokee_mysql_free_result (&$result)
{
	if (!_is_result($result))
	   return _mysql_free_result ($result);
	return TRUE;
}


/* mysql_get_client_info - Get MySQL client info */
function cherokee_mysql_get_client_info ()
{
	$info = _mysql_get_client_info();
	return $info . ' ' . CLIENT_INFO;
}


/* mysql_get_host_info - Get MySQL host info */
function cherokee_mysql_get_host_info ($link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_get_host_info', $link);
	}

	$info = $link['db_host'] . 'via HTTP';
	return $info;
}


/* mysql_get_proto_info - Get MySQL protocol info */
function cherokee_mysql_get_proto_info ($link = NULL)
{
	if (_dbslayer($link))
	   $link = NULL;
	return _call('_mysql_get_proto_info', $link);
}


/* mysql_get_server_info - Get MySQL server info */
function cherokee_mysql_get_server_info ($link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_get_server_info', $link);
	}
	
	return SERVER_INFO;
}


/* UNIMPLEMENTED: mysql_info - Get information about the most recent query */
function cherokee_mysql_info ($link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_info', $link);
	}
}


/* mysql_insert_id - Get the ID generated from the previous INSERT operation */
function cherokee_mysql_insert_id ($link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_insert_id', $link);
	}

	if ($link['success'] != FALSE)
	   return $link['insert_id'];
	return FALSE;
}


/* mysql_list_dbs - List databases available on a MySQL server */
/* Deprecated alias: mysql_listdbs */
function cherokee_mysql_list_dbs ($link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_list_dbs', $link);
	}

	$query = "SHOW DATABASES;";
	$result = cherokee_mysql_query($query, $link);

	if ($link['success']) 
	   return $result;
	return FALSE;
}


/* mysql_list_fields - List MySQL table fields */
/* Deprecated alias: mysql_listfields */
function cherokee_mysql_list_fields ($database, $table_name, $link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_list_fields', $database_name, $table_name, $link);
	}

	cherokee_mysql_select_db($database);
	$query = "SHOW COLUMNS FROM $table_name";
	$result = cherokee_mysql_query($query, $link); 
	$result['_dbslayer']['_generator'] = 'list_fields';

	return $result;
	// to use with: cherokee_mysql_field_flags(), cherokee_mysql_field_len(),
	//	  	cherokee_mysql_field_name(),  cherokee_mysql_field_type()
}


/* mysql_list_processes - List MySQL processes */
function cherokee_mysql_list_processes ($link = NULL) {
	if (!_dbslayer($link)) {
	   return _call('_mysql_list_fields', $database_name, $table_name, $link);
	}

	cherokee_mysql_select_db($database);
	$query = "SHOW PROCESSLIST";
	$result = cherokee_mysql_query($query, $link); 

	$params = _get_params($result);

	if ($params['_idxres'] < 0)
   	   return FALSE;

	return $result;
}


/* mysql_list_tables - List tables in a MySQL database */
function cherokee_mysql_list_tables  ($database, $link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_list_tables', $database, $link);
	}

	$query = "SHOW TABLES FROM $database";
	$result = cherokee_mysql_query($query, $link); 
	$result['_dbslayer']['_generator'] = 'list_tables';

	return $result;
	// to use with: cherokee_mysql_tablename()
}


/* mysql_num_fields - Get number of fields in result */
function cherokee_mysql_num_fields ($result)
{
	if (!_is_result($result))
	   return _mysql_num_fields ($result);

	if (!isset($result["_dbslayer"]["_fields"]))
	   return FALSE;

	return $result["_dbslayer"]["_fields"];
}


/* mysql_num_rows - Get number of rows in result */
function cherokee_mysql_num_rows ($result)
{
	if (!_is_result($result))
	   return _mysql_num_rows ($result);
	if (!isset($result["_dbslayer"]["_rows"]))
	   return FALSE;
	return $result["_dbslayer"]["_rows"];
}


/* mysql_ping - Ping a server connection  */
function cherokee_mysql_ping ($link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_ping', $link);
	}

	$proc = cherokee_mysql_list_processe($link);
	if ($proc)
	   return TRUE;

	return FALSE;
}


/* mysql_pconnect - Open a persistent connection to a MySQL server */
function &cherokee_mysql_pconnect ($host = NULL, $user = NULL,
			     $pass = NULL, $client_flags = 0)
{
	global $_dbslayer;

	$host = ($host) ? $host:$_dbslayer['db_host'];
    	$user = ($user) ? $user:$_dbslayer['db_user'];
    	$pass = ($pass) ? $pass:$_dbslayer['db_pass'];

	if ($host != $_dbslayer['db_host'])
	   return _mysql_pconnect($host, $user, $pass, $client_flags);

	$_dbslayer['db_host'] = $host;
    	$_dbslayer['db_user'] = $user;
    	$_dbslayer['db_pass'] = $pass;

	return $_dbslayer;
}


/* mysql_query - Send a MySQL query */
function cherokee_mysql_query ($query, $link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_query', $query, $link);
	}

	global $_dbslayer;

	if ($_dbslayer['db_name'])
	   $query = "use " . $_dbslayer['db_name'] . "; " . $query;

	$url = $_dbslayer['db_host'] . "/" . rawurlencode($query);
	if (!curl_setopt ($_dbslayer['curl'], CURLOPT_URL, $url)||
	    !curl_setopt ($_dbslayer['curl'], CURLOPT_RETURNTRANSFER, 1)) {
	   $err = "cannot set cURL CURLOPT_URL to " . $_dbslayer['db_host'].":" .
	   	   curl_error() . "(" . curl_errno() . ")";
	   throw new Exception($err);
	}

	$result = curl_exec ($_dbslayer['curl']);
	if (!$result) {
	   $err = "curl_exec() failed for url " . $_dbslayer['db_host'] . ":" . 
	   	   curl_error() . "(" . curl_errno() . ")";
	   throw new Exception($err);
	}

	eval("\$obj = $result;");

	$dbs = array(
		"_idxres" => count($obj) - 1, // points to the result field
		"_idxrow" => 0,	       	      // row pointer
		"_field_offset" => 0,         // field_offset for mysql_fetch_field
		"_last_access" => -1,	      // last accessed row
		"_generator"   => 'query');   // mysql originating function
	$obj["_dbslayer"] = $dbs;
	$_dbslayer['success'] = TRUE;

	// Map results or errors
	do {
	   $idx = $obj["_dbslayer"]["_idxres"];
	   $tmp = $obj[$idx];

	   if (isset($tmp["RESULT"])){
	      $obj["_dbslayer"]["_rows"]   = count ($tmp["RESULT"]["ROWS"]);
	      $obj["_dbslayer"]["_fields"] = count ($tmp["RESULT"]["HEADER"]);
	      return $obj;
	   }

	   if (isset($tmp["SUCCESS"]) && $tmp["SUCCESS"] == 0) {
	      $_dbslayer['success'] = FALSE;
	      $_dbslayer['errno'] = $tmp["MYSQL_ERRNO"];
	      $_dbslayer['error'] = $tmp["MYSQL_ERROR"];
	      $_dbslayer['affected_rows'] = -1;
	      break;
 	   }

	   if (isset($tmp["AFFECTED_ROWS"])) {
	      $_dbslayer['affected_rows'] = $tmp["AFFECTED_ROWS"];
	   }

	   if (isset($tmp["INSERT_ID"])) {
	      $_dbslayer['insert_id'] = $tmp["INSERT_ID"];
 	   }

	   $obj["_dbslayer"]["_idxres"]--;
	} while ($obj["_dbslayer"]["_idxres"] >=0);

	// Statements returning resultset return a resource on success, or FALSE.
	// Other type of SQL statements return TRUE on success or FALSE on error. 
	return $_dbslayer['success'];
}


/* mysql_real_escape_string - Escapes characters for use in a SQL statement */
/* Prepend backslashes to the : \x00, \n, \r, \, ', " and \x1a.             */
function cherokee_mysql_real_escape_string ($unescaped_string, $link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_real_escape_string', $unescaped_string, $link);
	}
	return _mysql_escape_string($unescaped_string);
}


/* mysql_result - Get result data. Fields can be numbers or
 * names. table.name notation not yet supported 
 */
function cherokee_mysql_result (&$result, $row, $field = 0)
{
	if (!_is_result($result))
    	   return _mysql_result ($result, $row, $field);
	if ($row < 0 || $row >= $result["_dbslayer"]["_rows"])
	   return NULL;
	
	$idx = $result["_dbslayer"]["_idxres"];
	$tmp = $result[$idx];
	if (!is_numeric($field)) {
	   $i=0;
	   foreach ($tmp["RESULT"]["HEADER"] as $value) {
	   	 if ($value == $field) break;
	   	 $i++;
	   }
	   if ($i < count($tmp["RESULT"]["HEADER"]))
	      $field = $i;
	   else
	      return NULL;
	}

	return $tmp["RESULT"]["ROWS"][$row][$field];
}


/* mysql_select_db - Select a MySQL database */
function cherokee_mysql_select_db ($database_name, $link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_select_db', $database_name, $link);
	}

	global $_dbslayer;
	$_dbslayer['db_name'] = $database_name;
	$link['db_name']      = $database_name;
	
	return TRUE;
}


/* mysql_set_charset - Sets the client character set */
function cherokee_mysql_set_charset ($charset , $link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_set_charset', $charset, $link);
	}

	$query = "SET character_set_results = '"  . $charset .
	         "', character_set_client = '"    . $charset .
		 "', character_set_connection = '". $charset .
		 "', character_set_database = '"  . $charset .
		 "', character_set_server = '"    . $charset .
		 "'";

	$result = cherokee_mysql_query($query, $link);
	$params = _get_params($result);

	if ($params['success'])
	   return TRUE;
	return FALSE;
}


/* mysql_stat - Get current system status */
function cherokee_mysql_stat ($link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_stat', $link);
	}

	$query  = "SHOW STATUS WHERE Variable_name = 'Uptime' OR "
		  	       	   ."Variable_name = 'Questions' OR "
		  	       	   ."Variable_name = 'Slow_queries' OR "
		  	       	   ."Variable_name LIKE 'Threads%' OR "
		  	       	   ."Variable_name LIKE 'Open%' OR "
		  	       	   ."Variable_name LIKE 'Flu%'";

	$result = cherokee_mysql_query($query, $link);

	$params = _get_params($result);

	if ($params['_idxres'] < 0)
	   return NULL;


	$info = "";
	while ($row = cherokee_mysql_fetch_assoc($result)) {
	      $info .= $row['Variable_name'] . ": " . $row['Value'] . "  ";
	}

	return trim($info);
}


/* mysql_tablename - Get table name of field */
function cherokee_mysql_tablename ($result, $i)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_tablename', $result, $i);
	}

	$params = _get_params($result);
	if ($params['_generator'] != 'list_tables')
	   return FALSE;

	if ($i < 0 || $i >= $params['_rows'])
	   return FALSE;
	
	$rows = _get_rows($result);

	return $rows[$i][0];
}


/* UNIMPLEMENTED: mysql_thread_id - Return the current thread ID */
function cherokee_mysql_thread_id ($link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_mysql_thread_id', $link);
	}

	return FALSE;
}


/* mysql_unbuffered_query - Send an SQL query to MySQL, without fetching and buffering the result rows */
function cherokee_mysql_unbuffered_query ($query, $link = NULL)
{
	if (!_dbslayer($link)) {
	   return _call('_mysql_unbuffered_query', $query, $link);
	}

	return cherokee_mysql_query($query, $link);
}


class Mysql_fetch_field_object 
{
      public $name;         // column name
      public $type;         // the type of the column
      public $blob;         // 1 if the column is a BLOB
      public $numeric;      // 1 if the column is numeric

      /* These not yet provided:
      public $table;        // name of the table the column belongs to
      public $def;          // default value of the column
      public $max_length;   // maximum length of the column
      public $not_null;     // 1 if the column cannot be NULL
      public $primary_key;  // 1 if the column is a primary key
      public $unique_key;   // 1 if the column is a unique key
      public $multiple_key; // 1 if the column is a non-unique key
      public $unsigned;     // 1 if the column is unsigned
      public $zerofill;     // 1 if the column is zero-filled
     */
}

?>
