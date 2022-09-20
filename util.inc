<?PHP
define ('L_DEBUG1', 0);
define ('L_DEBUG', 1);
define ('L_INFO', 2);
define ('L_WARN', 3);
define ('L_ERR', 4);
define ('L_CRIT', 5);

define ('L_DIE', 128);
define ('L_HTML', 256);


global $log_level;

date_default_timezone_set('Europe/Prague');

function tolog($str, $level=L_DEBUG) {
        global $log_level;
        $d=date('YmdHis');
        $bk=debug_backtrace();
        $c=count($bk);
        $bktr='main';
        for ($i=$c-1; $i>0; $i--) $bktr.="/".$bk[$i]['function'];
        if (($level & 127)>=$log_level || $level & L_DIE) {
                if ($log_level & L_HTML) {
                        if ($level & L_DIE) throw_error(500, "$bktr: $str");
                        echo "<br /><br /><b>$d:</b> <tt>$bktr: $str</tt><br />\n";
                } else echo "$d: $bktr: $str\n";
        }

        if ($level & L_DIE) die();
}



function mail_error($class, $subjkey, $msg) {
    mail ('tomas@simek.info', "$class: $subjkey", $msg);
    mail ('admin@necoss.net', "$class: $subjkey", $msg);
    mail ('podpora@necoss.net', "$class: $subjkey", $msg);
}

function mf($n, $l=1, $d=2) {
	$s=number_format($n, $d, '.', ' ');
	$sp='';
	while ($l>=strlen($s)) {
		$sp.=' ';
		$l--;
	}
	return ($sp.$s);
}

//naparsuje do SQL vstup od usera
//napr "jmeno:s,prijmeni:sm,id:im"
function request_formdata($name, $attr, $ignore_must=0, $die_error=1) {
//echo "request_formdata($name, $attr, $ignore_must=0, $die_error=1)";
	if (substr($attr, 0, 1)=='e') { //existence, napr pro checkbox
	    if (isset($_REQUEST[$name])) return (1); else return(0);
		}
	if ((substr($attr, 1, 1)=='o') && !isset($_REQUEST[$name])) {
		return; //povoleni ignorovat
		}
	if ((substr($attr, 1, 1)=='z') && !isset($_REQUEST[$name])) {
		return(''); //povoleni ignorovat
		}
	if (!isset($_REQUEST[$name])) {
	    if (!$ignore_must && (substr($attr, 1, 1)=='m') && $die_error) die("error requested attribute $name not sent");
		return('+===+error');
	}
	
	switch (substr($attr, 0, 1)) {
		case i:
			$val=floor($_REQUEST[$name]);
//			echo "fffffffffffffffffff$name $val";
			break;
		case d: //pouzito pro id
			$val=$_REQUEST[$name];
			if ($val=='NULL') return(NULL);
			$val=floor($val);
			if ($val<1)
				die("request_sql:$name=$val: this is not id");
			break;
		case n: //pouzito pro id, nic=NULL
			$val=$_REQUEST[$name];
			if ($val=='NULL') return(NULL);
			if (!$val) return(NULL);
			$val=floor($val);
			if ($val<1)
				die("request_sql:$name=$val: this is not id");
			break;
		case f:
			$val=(float)$_REQUEST[$name];
			break;
		case x:
			$val=$_REQUEST[$name];
			break;
		case s:
			$val=addslashes(stripslashes($_REQUEST[$name]));
			break;
		case k:  // pouzito pro SET, dokaye handle array
			$val='';
			foreach($_REQUEST[$name] as $v) {
				if ($val) $val="$val,";
				$val=$val.addslashes($v);
				}
			break;
	}
	return($val);
}

function request_sql($name, $attr, $ignore_must=0, $die_error=1) {
    $val=request_formdata($name, $attr, $ignore_must, $die_error);
    if (is_null($val)) return("`$name`=NULL");
    if ((string)$val=="+===+error") {
		return;
    }
    return("`$name`='$val'");
}

function request_sql_set($flags, $ignore_must=0, $die_error=1) {
	$f=explode(',', $flags);
	for ($i=0; $i<count($f); $i++) {
		if ($f=='') continue;
		list($name, $attr)=explode(':', $f[$i]);
		$tag=request_sql($name, $attr, $ignore_must, $die_error);
		if ($tag) {
			if ($sql) $sql="$sql, ";
			$sql=$sql.$tag;
		}
	}
	return ($sql);
}

function request_fdarray_set ($flags, $ignore_must=0, $die_error=1) {
	$f=explode(',', $flags);
	for ($i=0; $i<count($f); $i++) {
		if ($f=='') continue;
		list($name, $attr)=explode(':', $f[$i]);
		$tag=request_formdata($name, $attr, $ignore_must, $die_error);
		if ($tag) $data[$name]=$tag;
	}
	return ($data);
}

function request_fdarray_sql ($flags, $table, $cmdrest) {
	global $db;
	$f=explode(',', $flags);
	$cols='';
	foreach ($f as $t) {
		list($col)=explode(':', $t);
		if ($col=='') continue;
		if ($cols) $cols.=', ';
		$cols.=$col;
	}
	$res=$db->tsquery("SELECT $cols FROM $table $cmdrest", "request_fdarray_sql: $table");
	if (!$ret=fetch_a($res)) return(array());
	return ($ret);
}

function format_czechnum($num, $one, $few, $many){
	if ($num==0) return("$num $many");
	if ($num==1) return("$num $one");
	if (round($num)!=$num) return ("$num $few");
	if ($num<5) return ("$num $few");
	return ("$num $many");
}


function sequence_get($name) {
	global $db;
	$db->tsquery("LOCK TABLES sequences WRITE", "sequence_get:zamknout tabulku sequences");
	$res=$db->tsquery("INSERT INTO sequences SET seq_name='$name', seq_max=1 ON DUPLICATE KEY UPDATE seq_max=seq_max+1",
		"sequence_get:nastavit/zvysit klic");
	$res=$db->tsquery("SELECT seq_max FROM sequences WHERE seq_name='$name'",
		"sequence_get:vybrat klic");
	$key=$res->fetchColumn();
	if (!$key) die("sequence_get:klic je divny");
	$db->tsquery("UNLOCK TABLES", "sequence_get:odemknout tabulku sequences");
	return($key);
}

function sequence_check($name) {
	global $db;
	$res=$db->tsquery("SELECT seq_max FROM sequences WHERE seq_name='$name'", 'sequence_get:vybrat klic');
	$key=$res->fetchColumn();
	return($key);
}

function sequence_set($name, $new=0) {
	global $db;
	$new=$new+0;
	$res=$db->tsquery("UPDATE sequences SET seq_max=$new WHERE seq_name='$name'", 'sequence_get:zapsat klic');
}

function persistent_var($name, $dbname=NULL, $default=NULL, $persist=0){
	list($name, $attr)=explode(':', $name);
	if (!$dbname) $dbname=$name;
	$get_val=request_formdata($name, $attr);
	if (!is_null($get_val)) pref_set($dbname, $get_val, $persist);
	$v=pref_get($dbname);
	if (is_null($v)) $v=$default;
//	echo "[$name, $dbname, $v, $get_val]";
	return($v);
}

function formdate($sql) {
	list($d)=explode(' ', $sql);
	list($y, $m, $d)=explode('-', $d);
	$y+=0;
	$m+=0;
	$d+=0;
	return("$d.$m.$y");
}




?>