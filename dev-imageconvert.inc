<?PHP
/*
Debug
require_once(__DIR__.'/../module_setting.inc');
require_once($fs_root."/fwlib/mysql-pdo.inc");
require_once($fs_root."/fwlib/util.inc");
db_establish_local($mysql_user, $mysql_pass, $mysql_db, $mysql_host);
$log_level=L_DEBUG1;
*/
function image_get_variants($path, $module) {
	global $db;
	// delete also disabled ones! (Feature, not a bug)
	$res=$db->tsquery("SELECT * FROM image_preprocessing_rules WHERE ir_which='$module' ORDER BY ir_order", 'get rules');
	$ret=[];
	while ($r=fetch_o($res)) {
		$dir=dirname($path);
		$fn=basename($path);
		if ($r->ir_out_regexp_match) $fn=preg_replace($r->ir_out_regexp_match, $r->ir_out_regexp_replace, $fn);
		$ret[]=$dir.'/'.$r->ir_files_prefix.$fn;
		if ($r->ir_enable_compat_prefix) 
			$ret[]=$dir.'/'.$r->ir_compat_prefix.$fn;
	}
	return($ret);
}

function image_delete_variants($path, $module) {
	$file_names=image_get_variants($path, $module);
	foreach ($file_names as $fn)
		unlink($fn);
}

function image_process_variants($path, $module) {
	global $db;
	$res=$db->tsquery("SELECT * FROM image_preprocessing_rules WHERE ir_enabled AND ir_which='$module' ORDER BY ir_order", 'get rules');
	while ($r=fetch_o($res)) {
		tolog("processing $r->ir_comment on $path", L_DEBUG);
		$dir=dirname($path);
		$fn=basename($path);
		if ($r->ir_out_regexp_match) $fn=preg_replace($r->ir_out_regexp_match, $r->ir_out_regexp_replace, $fn);
		$np=$dir.'/'.$r->ir_files_prefix.$fn;
		if ($r->ir_preprocessor_cmd) {
			$cmd=str_replace('{out}', $np, str_replace('{in}', $path, $r->ir_preprocessor_cmd));
			tolog ("executing $cmd", L_DEBUG1);
			exec($cmd);
		}
		if ($r->ir_unlink_compat_prefix) {
			$ap=$dir.'/'.$r->ir_compat_prefix.$fn;
			tolog("rm $ap", L_DEBUG1);
			unlink($ap);
		}
		
		if ($r->ir_enable_compat_prefix) {
			$ap=$dir.'/'.$r->ir_compat_prefix.$fn;
			tolog("linking $np, $ap", L_DEBUG1);
			symlink($np, $ap);
		}
	}
	return(true);
}

/*
print_r(image_get_variants('/var/www/V2api/output/blobs/poster/s12/s36/123696.jpg', 'media-posters'));
image_process_variants('/var/www/V2api/output/blobs/poster/s12/s36/123696.jpg', 'media-posters');
*/
