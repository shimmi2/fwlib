<?PHP
function get_set_string($table, $column) {
	global $db;
	$res=$db->tsquery("SHOW FIELDS FROM $table WHERE FIELD='$column'", "describe $column DB field");
	$t=fetch_o($res);
	$ret=str_replace("'", '', 
			str_replace(')', '', 
				str_replace('set(', '', 
					str_replace('enum(', '', $t->Type))));
	return($ret);
}

function checkdo_timer($task_name, $inteval, $mark=1, $whoami=1) {
	global $db;
	$do=FALSE;
	
	if (!db_trivialquery("SELECT ltd_id FROM last_task_timers WHERE ltd_name='$task_name' AND 
		ltd_doneby_$mask<DATE_SUB(NOW(), INTERVAL $inteval", "check checkdo_timer $task_name")) {
		if ($mark) 
			db_trivialupdate("INSERT INTO last_task_timers SET ltd_name='$task_name', ltd_doneby_$mark=NOW()", 'set checkdo_timer $task_name');
		return(TRUE);
	}
	return(FALSE);
}
			
// Never change this (we generates manifests on the fly from id based on this, so You can swcrew data
/* this was repuslive :)
function get_epg_hashed_path ($id) {
        if (!$id) return(FALSE);
        $maxdircount=1000; // Never change!!!
        $decades=floor(log($id, $maxdircount));
        $path='';
        for ($i=$decades; $i>0; $i--) {
                $base=pow($maxdircount, $i);
                $index=(intdiv($id, $base)) % $maxdircount;
                $path.="/s$index";
        }
        $path.='/'.$id;
        return($path);
}
*/
function get_epg_hashed_path($id) {
        if (!$id) return(FALSE);
        $maxdircount=1000; // Never change!!!
        $path="/$id";
        $index=intdiv($id, $maxdircount);
        while ($index>0) {
                $part=$index % $maxdircount;
                $path="/s$part$path";
                $index=intdiv($index, $maxdircount);
        }
        return($path);
}

function get_media_all_info ($mid, $mediainf=TRUE, $tags=TRUE, $images=TRUE) {
	global $db;
	$ret=[];
	if (!$mid) return(NULL);
	if ($mediainf) {
		$res=$db->tsquery("SELECT md_original_name FROM media WHERE md_id=$mid", 'get media');
		if ($d=fetch_o($res))
			$ret['original_name']=$d->md_original_name; else $ret['original_name']=NULL;
	}
	
	if ($tags) {
		$res=$db->tsquery("SELECT mt_mean, mt_value, mt_key, mti_description, mti_shortenfactor, mta_source
FROM media_tags_assignation 
INNER JOIN media_tags ON mta_tag=mt_id
LEFT JOIN media_tag_info ON mti_tag_mean=mt_mean
WHERE mta_media=$mid
GROUP BY mt_id
ORDER BY mti_order, mt_mean, mt_value", 'get media tags');
		$ret['media_tags']=[];
		$i=-1;
		$ot=NULL;
		$count=0;
		while ($t=fetch_o($res)) {
			if ($ot!=$t->mt_mean) {
				$i++;
				$j=0;
				$ot=$t->mt_mean;
				$ret['media_tags'][$i]=[ 'mean' => $t->mt_mean, 'description' => $t->mti_description, 
					'mti_shortenfactor' => $t->mti_shortenfactor, 'source' => $t->mta_source ];
			}
			$ret['media_tags'][$i]['v'][$j]['k']=$t->mt_key;
			$ret['media_tags'][$i]['v'][$j]['v']=$t->mt_value;
			$j++;
			$count++;
		}
		$ret['media_tags_count']=$count;
	}

	if ($images) {
		$res=$db->tsquery("SELECT * FROM media_gallery WHERE mg_media=$mid ORDER BY mg_order", 'fetch media');
		$g=0;
		$p=0;
		$s=0;
		$ret['posters']=[];
		$ret['gallery']=[];
		$ret['snapshots']=[];
		while ($m=fetch_o($res)) {
			$u=get_doc_uri($m->mg_id, $m->mg_type, $m->mg_extension);
			switch ($m->mg_type) {
				case 'poster':
					$ret['posters'][$p]['uri']=$u;
					$ret['posters'][$p]['id']=$m->mg_id;
					$p++;
					break;
				case 'snapshot':
					$ret['snapshots'][$s]['uri']=$u;
					$ret['snapshots'][$s]['id']=$m->mg_id;
					$s++;
					break;
				default:
					$ret['gallery'][$g]['uri']=$u;
					$ret['gallery'][$g]['id']=$m->mg_id;
					$g++;	
			}
		}
		$ret['posters_count']=$p;
		$ret['gallery_count']=$g;
		$ret['snapshots_count']=$s;

		// V1 compat magic and simple selector
		$ret['gallery_count']=$g;
		if ($p) $ret['epg_program_imageuri']=$ret['posters'][0]['uri'];
			else if ($g) $ret['epg_program_imageuri']=$ret['gallery'][0]['uri'];
				else if ($s) {
					$s=intval($s/2);
					$ret['epg_program_imageuri']=$ret['snapshots'][$s]['uri'];
				} else $ret['epg_program_imageuri']=NULL;
	}
	return($ret);
}

function get_doc_uri($id, $type, $extension) {
	global $hashstore_uri_https;
	if (!$path=doc_get_path($id)) return(FALSE);
	return("$hashstore_uri_https/$type/$path".$extension);
}

