<?PHP
// Expect $hash_store_prefix

function check_store($log=L_ERR | L_HTML | L_DIE) {
	global $hashstore_prefix;
	if (!$hashstore_prefix) {
		tolog("hashstore: get_object: hashstore_prefix invalid", $log);
		return(FALSE);
	}
	if (!file_exists("$hashstore_prefix/_-_-_-hashstore_archive_signature.mark")) {
		tolog("FATAL: hashstore: no archive signature (_-_-_-hashstore_archive_signature.mark) found in path ($hashstore_prefix)", $log);
		return(FALSE);
	}
	return(TRUE);
}


// Never change this! Or You break all archive (we don't keep way in DB, its just hashed)
// up to 2*$hashstore_prefix objects can be in dir (hashstore_prefix files, hashstore_prefix dirs)
function doc_get_path($id) {
	global $hashstore_prefix;
	if (!$id) return(FALSE);
	$maxdircount=100; // Never change!!!
	$decades=floor(log($id, $maxdircount));	
	$path='';
	for ($i=$decades; $i>0; $i--) {
		$base=pow($maxdircount, $i);
		$index=(intdiv($id, $base)) % 100;
		$path.="/s$index";
	}
	$path.='/'.$id;
	return($path);
}

function doc_get_full_path($id, $class, $extension) {
	global $hashstore_prefix;
	$path=doc_get_path($id);
	return("$hashstore_prefix/$class/$path".$extension);
}

function get_object($class, $id, $extension='', $log=L_ERR | L_HTML | L_DIE) {
	global $hashstore_prefix;
	if (!check_store($log)) return(FALSE);
	if (!$path=doc_get_path ($id)) return(FALSE);
	$name="$hashstore_prefix/$class/$path".$extension;
	
	if (!$data=file_get_contents($name)) {
		tolog("get_object fatal: $name not readable", $log);
		return(NULL);
	}
	return($data);
}

// id is int!
function doc_store($class, $id, $extension='', $data, $mask=0755, $log=L_ERR | L_HTML | L_DIE) {
	global $hashstore_prefix;
	if (!check_store($log)) return(FALSE);
	if (!$path=doc_get_path($id)) return(FALSE);
	$name="$hashstore_prefix/$class/$path".$extension;
	$dir=dirname($name);
	umask(0);
	if (!file_exists($dir)) mkdir($dir, $mask, true);
	umask(0111);
//	if (!file_put_contents("$root/$name", $data)) die("documents.inc: doc_store: can't write file to $root/$name");
	$f = @fopen($name, 'w');
    if (!$f) {
		tolog("doc_store: can't fopen file to $name", $log);
		return(FALSE);
	}
	$bytes = fwrite($f, $data);
    fclose($f);
    if (!$bytes) {
		tlog("doc_store: can't write to $name: ".posix_strerror(posix_get_last_error()), $log);
		return(FALSE);
	}
	return(TRUE);
}

function doc_delete($class, $id, $extension='', $log=L_ERR | L_HTML | L_DIE) {
	if (!check_store($log)) return(FALSE);
	if (!$path=doc_get_path ($id)) return(FALSE);
	$name="$hashstore_prefix/$class/$path".$extension;
	if (!unlink($name)) {
		tolog("doc_store: can't delete $name", $log);
		return(FALSE);
	}
	try {
		// three dirs up should be enough for cleanup
		$dir=dirname($name);
		rmdir($dir);
		$dir=dirname($name);
		rmdir($dir);
		$dir=dirname($name);
		rmdir($dir);
	} catch (Exception $e) {
	}
	return(TRUE);
}
	


?>