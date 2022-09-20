<?PHP
global $db;
global $pdb;

class TomPDO extends PDO
{
    public function tsquery($q, $where="[neznamo]", $noexit=0) {
        $pdos=parent::query($q);
        if (!$pdos) {
			$arr=parent::errorInfo(); 
			tolog("$where: PDO cmd error: $arr[2]", L_ERR);
			if ($noexit) return(NULL);
			exit(-1);		
		} else $arr=$pdos->errorInfo();
		if ($arr[2]) {
			tolog("$where: PDO stat error: $arr[2]", L_ERR);
			if ($noexit) return(NULL);
			exit(-1);
		}
		return($pdos);
    }
 }

function dberror($where, $db=NULL) {
	tolog("$where: fatal code issue You need rewrite dberror(where, db) to ndberror (where, PDOstatement)", L_ERR);
}

function ndberror($where, $sth) {
	$arr=$sth->errorInfo();
	if ($arr[2]) {
		tolog("$where: PDO error: $arr[2]", L_ERR);
		exit(-1);
	}
}

function db_trivialquery($sql, $desc="[neznamo]", $remote=0, $noexit=0) {
	global $db;
	global $pdb;
	if ($remote) $ddb=&$pdb; else $ddb=&$db;
	$res=$ddb->tsquery($sql, $desc, $noexit);
	if (!$res) return(NULL);
	$c=$res->rowCount();
	if (!$c) {
		unset($res);
		return(NULL);
	}
	$r=$res->fetchColumn();
	unset($res);
	return ($r);
}

function db_trivialupdate($sql, $desc="[neznamo]", $remote=0, $noexit=0) {
	global $db;
	global $pdb;
//	echo "$sql | $desc | $remote\n";
	if ($remote) $ddb=&$pdb; else $ddb=&$db;
	$res=$ddb->tsquery($sql, $desc, $noexit);
	if (!$res) return(NULL);
	$c=$res->rowCount();
	unset($res);
	return($c);
}

function db_trivialinsert($sql, $desc="[neznamo]", $remote=0, $noexit=0) {
	global $db;
	global $pdb;
//	echo "$sql | $desc | $remote\n";
	if ($remote) $ddb=&$pdb; else $ddb=&$db;
	$res=$ddb->tsquery($sql, $desc, $noexit);
	if (!$res) return(NULL);
	$c=$db->lastInsertId();
	unset($res);
	return($c);
}


function db_establish_local($db_u, $db_p, $dbname='startvcodec', $host='localhost') {
	global $db;
	try {
		$db = new TomPDO("mysql:host=$host;dbname=$dbname;charset=utf8", $db_u, $db_p);
	} catch (PDOException $e) {
		tolog ('local db connection failed: '.$e->getMessage(), L_ERR);
		die();
	}
	$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_SILENT);
}

function db_establish_v2() {
	global $db, $mysql_host, $mysql_user, $mysql_pass, $mysql_db;
	try {
		$db = new TomPDO("mysql:host=$mysql_host;dbname=$mysql_db;charset=utf8", $mysql_user, $mysql_pass);
	} catch (PDOException $e) {
		tolog ('V2 db connection failed: '.$e->getMessage(), L_ERR);
		die();
	}
	$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_SILENT);
}



// if original mysql exists this hack is unusable and code must be rewritten
if (!function_exists('mysql_query')) {
	
	function mysql_query($q, $exdb=NULL) {
		global $db;
		if ($exdb) return($exdb->query($q));
		return($db->query($q));
	}

	function mysql_fetch_object(&$res) {
		return($res->fetch(PDO::FETCH_OBJ));
	}

	function mysql_fetch_assoc(&$res) {
		return($res->fetch(PDO::FETCH_ASSOC));
	}
}

function fetch_o(&$res) {
	return($res->fetch(PDO::FETCH_OBJ));
}

function fetch_a(&$res) {
	return($res->fetch(PDO::FETCH_ASSOC));
}


function db_establish_platform($platform) {
	global $db;
	global $pdb;
	$resp=$db->tsquery("SELECT * FROM platforms WHERE pl_id=$platform", "fetch platform");
	if (!$p=$resp->fetch(PDO::FETCH_OBJ)) die("slot not found");

	try {
		$pdb=new TomPDO("mysql:host=$p->pl_ip;dbname=startv;charset=utf8", $p->pl_sqluser, $p->pl_sqlpassword);
	} catch (PDOException $e) {
		tolog ('platform database connect failed: '.$e->getMessage(), L_ERR);
		die();
	}
}

function db_sql_all_insert($db_link, $table, $varname, $ignore='', $fmt='object') {
    $res=$db_link->tsquery("SELECT * FROM $table LIMIT 1", "describe table $table");
    $ret='';
	$ignore=explode(',', $ignore);
	for ($i= 0; $i < $res->columnCount(); $i++) {
		$col= $res->getColumnMeta($i);
		$col=$col['name'];
		if (array_search($col, $ignore)) continue;
		if ($ret) $ret.=', ';
		switch ($fmt) {
			case 'object':
			default:
				$ret.="`$col`='\$$varname"."->$col'";
				break;
			case 'assoc':
 				$ret.="`$col`='\$$varname"."[$col]'";
				break;
		}
   }
	unset($res);
    return($ret);
}

global $__db_t_cache;
$__db_t_cache=[];

function db_sql_all_insert2($db_link, $table, &$var, $ignore='', $fmt='object') {
	global $__db_t_cache;
	if(isset($__db_t_cache[$table])) $table_desc=$__db_t_cache[$table];
	else {
		$res=$db_link->tsquery("SELECT * FROM $table LIMIT 1", "describe table $table");
		$table_desc=[];
		for ($i= 0; $i < $res->columnCount(); $i++) {
			$col=$res->getColumnMeta($i);
			$table_desc[$col['name']]=$col;
		}
		$__db_t_cache[$table]=$table_desc;
		unset($res);
	}
	
	$ret='';
	$ignore=explode(',', $ignore);
	
	foreach ($table_desc as $colname => $cd) {
		if (array_search($colname, $ignore)) continue;
		if ($ret) $ret.=', ';
		switch ($fmt) {
			case 'object':
			default:
				$val=$var->$colname;
				break;
			case 'assoc':
 				$val=$var[$colname];
				break;
		}
		if ($val===NULL) {
			$ret.="`$colname`=NULL";
			continue;
		}
		
		switch ($cd['native_type']) {
			case 'BIT': case 'TINY': case 'SHORT': case 'LONG': case 'FLOAT': case 'DOUBLE':
					$ret.="`$colname`=$val";
				break;
			default:
				$ret.="`$colname`='$val'";
		}
	}
	return($ret);
}


function dblog($what, $src='[není známo]', $type='system', $severity='info', $sess='', $user='') {
	global $db, $session;
	if ($sess) $session=$sess;
	if ($user) $q=", log_user=$user"; else $q='';
	$res=$db->query("INSERT INTO log SET `log_when`=NOW(), log_type='$type', log_what='$what', log_src='$src', log_session='$session', log_severity='$severity'$q");
	$arr=$res->errorInfo();
	if ($arr[2]) die("$where: PDO error: $arr[2]"); // neni pouzite dberror kbuli reentracni
}

function get_set_values($table, $col) {
	global $db;
	$res=$db->tsquery("DESCRIBE $table", 'get set attrs');
	while ($d=fetch_a($res)) {
		if ($d['Field']==$col) {
			$set=$d['Type'];
			break;
		}
	}
	$set=preg_replace("/^set\(\'/", '', $set);
	$set=preg_replace("/^enum\(\'/", '', $set);
	$set=preg_replace("/'\)$/", '', $set);
	$ar=explode("','", $set);
	return($ar);
}

function db_dup_line_except($db, $table, $except, $where, $add_cols='', $add_vals='') {
	$res=$db->tsquery("SELECT * FROM $table LIMIT 1", "describe table $table");
    $sql='';
	$ignore=explode(',', $except);
	for ($i= 0; $i < $res->columnCount(); $i++) {
		$col=$res->getColumnMeta($i);
		$col=$col['name'];
		if (array_search($col, $ignore)!==FALSE) continue;
		if ($sql) $sql.=', ';
		$sql.=$col;
   }
	$db->tsquery("INSERT INTO $table ($sql, $add_cols) SELECT $sql, $add_vals FROM $table WHERE $where", 'dup row');
	return($db->lastInsertId());
}
?>