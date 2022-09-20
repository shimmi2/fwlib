<?PHP
	global $db, $prof_func, $prof_start, $cachehit;
function profiler_start($f='default') {
	global $db, $prof_func, $prof_start;
	$prof_start=microtime(true)*1000.0;
	$prof_func=$f;
	return($prof_start);
}

function profile_store($f='', $t=0, $cachehit=0) {
	global $db, $prof_func, $prof_start, $cachehit, $sid;
	if (!$f) $f=$prof_func; 
	if (!$t) $t=$prof_start;
	if ($cachehit) $cachehit=1; else $cachehit=0;
	$t=microtime(true)*1000.0-$t;
	if ($t<0) return(-1);
	if ($t>10000 && isset($sid->us_id) && $sid->us_id) 
		db_trivialupdate("UPDATE users SET us_long_calls_count=us_long_calls_count+1 WHERE us_id=$sid->us_id", 'increment us_long_calls_count');
	
	$db->tsquery("
INSERT INTO profiler SET 
	prof_function='$f', prof_mintime=$t, prof_avg1000=$t, prof_maxtime=$t, prof_count_actual=1, 
	prof_avg_count_10s=1.0, prof_cntto1000=1, prof_count_cached=$cachehit
ON DUPLICATE KEY UPDATE
	prof_cntto1000=IF(prof_cntto1000<1000, prof_cntto1000+1, prof_cntto1000),
	prof_mintime=IF($t<prof_mintime, $t, prof_mintime),
	prof_maxtime=IF($t>prof_maxtime, $t, prof_maxtime),
	prof_count_actual=prof_count_actual+1,
	prof_avg1000=prof_avg1000-prof_avg1000/prof_cntto1000+$t/prof_cntto1000,
	prof_count_cached=prof_count_cached+$cachehit",
	'write profiler data');
}
