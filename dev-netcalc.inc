<?PHP

function ip_to_hex($ip){
    list($a1, $a2, $a3, $a4)=explode('.', $ip);
    return ($a1<<24 | $a2<<16 | $a3<<8 | $a4);
}

function hex_to_ip($hex){
    $a1=$hex & 0xff;
    $a2=$hex>>8 & 0xff;
    $a3=$hex>>16 & 0xff;
    $a4=$hex>>24 & 0xff;
    return("$a4.$a3.$a2.$a1");
}

function chk_ip($ip, $net='') {
    if (!preg_match("/^([0-9]{1,3}\.){3,3}[0-9]{1,3}(\/[0-9]{1,2})?$/", $ip)) return(0);
    $net=explode('/', $net);
    if (isset($net[1])) $mask=$net[1]; else $mask=24;
    $net=$net[0];
    if ($mask<0 || mask>32) return(0);
    $net_size=pow(2, 32-$mask);
    $net=ip_to_hex($net);
    list($ip, $ipmask)=explode('/', $ip);
    if (!$ipmask) $ipmask=32;
    if ($ipmask<0 || $ipmask>32) return(0);

    if ($ipmask<$mask) return(0);

    $ip=ip_to_hex($ip);
    $ip=$ip & (0xffffffff ^ ($net_size-1));
    $net=$net & (0xffffffff ^ ($net_size-1));
//    echo "masky   ".hex_to_ip($ip).' '.hex_to_ip($net).'<br>';
    return($ip==$net);
}

function chk_ip6($ip, $net='') {
	@list ($ip, $ipmask)=explode('/', $ip);
	if (!$ipmask) $ipmask=64;
	if ($ipmask>128 || $ipmask<0) return(0);
	if (!filter_var($ip, FILTER_VALIDATE_IP, FILTER_FLAG_IPV6)) return(0);

	if (!$net) return(1);
	
	@list ($netip, $netmask)=@explode('/', $net);
	if (!$netmask) $netmask=64;
	if ($netmask>128 || $netmask<0) return(0);
	if (!filter_var($netip, FILTER_VALIDATE_IP, FILTER_FLAG_IPV6)) return(0);
	
	if ($ipmask<$netmask) return(0);
	
	$m=dtr_pton(netmask_from_bits6($netmask));

//	echo "$ip, $netip\n";
	$ipnet=dtr_pton($ip);
	$netnet=dtr_pton($netip);
	$ipnet&=$m;
	$netnet&=$m;
//	echo "ipnet ".dtr_ntop($ipnet)."\n";
//	echo "netnet ".dtr_ntop($netnet)."\n";
	
	if ($ipnet==$netnet) return(1);
	return(0);
}

/**
 * dtr_pton
 *
 * Converts a printable IP into an unpacked binary string
 *
 * @author Mike Mackintosh - mike@bakeryphp.com
 * @param string $ip
 * @return string $bin
 */
function dtr_pton( $ip ){
	
	list($ip)=explode('/', $ip);
	
    if(filter_var($ip, FILTER_VALIDATE_IP, FILTER_FLAG_IPV4)){
        return current( unpack( "A4", inet_pton( $ip ) ) );
    }
    elseif(filter_var($ip, FILTER_VALIDATE_IP, FILTER_FLAG_IPV6)){
        return current( unpack( "A16", inet_pton( $ip ) ) );
    }
    return false;
}
/**
 * dtr_ntop
 *
 * Converts an unpacked binary string into a printable IP
 *
 * @author Mike Mackintosh - mike@bakeryphp.com
 * @param string $str
 * @return string $ip
 */
function dtr_ntop( $str ){
    if( strlen( $str ) == 16 OR strlen( $str ) == 4 ){
        return inet_ntop( pack( "A".strlen( $str ) , $str ) );
    }
    return false;
}



function chk_mac($mac) {
    $mac=str_replace('-', ':', $mac);
    if (preg_match("/^([0-9a-fA-F]{1,2}:){5,5}[0-9a-fA-F]{1,2}$/", $mac))
    return ($mac);
}

function calc_netaddr($ip, $add_ip=0) {
    if (!preg_match("/^([0-9]{1,3}\.){3,3}[0-9]{1,3}(\/[0-9]{1,2})?$/", $ip)) return(0);
    $net=explode('/', $ip);
    if (isset($net[1])) $mask=$net[1]; else $mask=24;
    $ip=$net[0];
    if ($mask<0 || $mask>32) return(NULL);

    $net_size=pow(2, 32-$mask);
    $net=ip_to_hex($ip);
    $net=$net & (0xffffffff ^ ($net_size-1));
	$net+=$add_ip;
    return(hex_to_ip($net)."/$mask");
}

function bits_from_mask6($mask){
	$p=0;
	if ($mask>=0 && $mask<=128) return($mask);
	
	foreach ($mask as $i => $mb) {
		if ($mb=='f' || $mb=='F') {
			$p++;
			continue;
		}
		if ($mb==':') continue;
		break;
	}
	if ($p>128) $p=128;
	return($p);
}

function get_mask6($mask) {
	if (!$mask) return(128);
	if (is_numeric($mask)) {
		if ($mask>128) return(128);
		if ($mask<0) return (0);
	}
	return(bits_from_mask6($mask));
}


function netmask_from_bits($bits) {
    $mask=pow(2, 32-$bits)-1;
    return(hex_to_ip(0xffffffff ^ $mask));
}

function netmask_from_bits6($bits) {
	if ($bits=='' || $bits>128 || $bits<0) return('0::0');
	if ($bits==128) return('ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff');
	$m='';
	$d='::';
	for ($last=16; $last<=128; $last+=16) {
		if ($m) $m.=':';
		if ($last==128) $d='';
		if ($bits<$last) return($m.strrev(sprintf("%04s", dechex((1 << ($bits % 16))-1))).$d);
		$m.='ffff';
	}
	return(NULL);
}

function normalize_ipv6($ip) {
	$ipm=explode('/', $ip);
	$tok=explode(':', $ipm[0]);
	$norm='';
	$i=0;
	$dot4=0;
	// lets handle ipv6net::ffff:ip.v.4.addr/mask notation, too
	foreach ($tok as $ii => $t) 
		if (!strcasecmp($t, 'ffff')) {
			if (!isset($tok[$ii+1])) break;
			$ip4=explode('.', $tok[$ii+1]);
			if (count($ip4)==4) {
				$h1=$ip4[0]<<8 | $ip4[1];
				$h2=$ip4[2]<<8 | $ip4[3];
				$tok[$ii+1]=dechex($h1);
				$tok[$ii+2]=dechex($h2);
			}
		}

	$c=count($tok);		
	foreach ($tok as $t) {
//echo "$i $t norm $norm\n";
		$l=strlen($t);
		if ($l) {
			for ($a=$l; $a<4; $a++) $t='0'.$t;
			if ($i) $norm.=':';
			$norm.=$t;
		} else {
			if ($i) 
				for ($a=$c-1; $a<8; $a++) $norm.=':0000';
			else
				$norm.='0000';
		}
		$i++;
	}
	if (isset($ipm[1])) $mask=get_mask6($ipm[1]); else $mask=128;
	return("$norm/$mask");
}

// in: 1:2:3:-:-:-:-:+:7:8
// out: 1:2:3::7:8
function __ipv6put($a) {
//	print_r($a);
	$r='';
	$c=count($a);
	foreach ($a as $i=>$x) {
		if ($x=='-' && !$i) $r.=':';		
		if ($x=='-') continue;
		if ($x=='+') {
			$r.=':';
			continue;
		}
		$r.=dechex(hexdec($x)+0);
		if ($i<$c-1) $r.=':';
		}
	return($r);
}

function minimize_ipv6($ip) {
	$ip=normalize_ipv6($ip);
	list($ip, $mask)=explode('/', $ip);
	$mask=get_mask6($mask);
	$ip=explode(':', $ip);
	$bl=array();
	$found=1;
	$start=0;
	$posibility=0;
	while ($found) {
		$found=0;
		for ($i=$start; $i<8; $i++) {
//echo "found $found i $i ip[$i]\n";
			if (!hexdec($ip[$i])) {
				$start=$i+1;
				$bl[$posibility]['s']=$i;
				$bl[$posibility]['c']=1;
				for ($j=$i+1; $j<8; $j++) {
					if (hexdec($ip[$j])) break;
					$bl[$posibility]['c']++;
					$start++;
				}
				$posibility++;
				$found=1;
				break;
			}
		}
	}
//	print_r($bl);
//	$bl=array(array('s' => 2, 'c' =>4));
	$max=0;
	$s=0;

	foreach($bl as $b) {
		if ($b['c']>$max) {
			$max=$b['c'];
			$s=$b['s'];
		}
	}

	for($i=$s; $i<$s+$max; $i++) 
		if ($i<$s+$max-1) $ip[$i]='-'; else $ip[$i]='+';
	return(__ipv6put($ip)."/$mask");
}

function add_addr6($addr1, $addr2) {
	list($a1, $m1)=explode('/', normalize_ipv6($addr1));
	list($a2, $m2)=explode('/', normalize_ipv6($addr2));
	$a1=explode(':', $a1);
	$a2=explode(':', $a2);
	$i=8;
	$sum='';
	$carry=0;
	while($i--){
		$s=hexdec($a1[$i])+hexdec($a2[$i])+$carry;
		if ($s>0xffff) $carry=1; else $carry=0;
		$sum=dechex($s & 0xffff).$sum;
		if ($i) $sum=':'.$sum;
	}
	return(minimize_ipv6("$sum/$m1"));
}

function and_addr6($addr1, $addr2, $normalized=0) {
	list($a1, $m1)=explode('/', normalize_ipv6($addr1));
	list($a2, $m2)=explode('/', normalize_ipv6($addr2));
	$a1=explode(':', $a1);
	$a2=explode(':', $a2);
	$i=8;
	$and='';
	while($i--){
		$s=dechex(hexdec($a1[$i]) & hexdec($a2[$i]));
		$and=$s.$and;
		if ($i) $and=':'.$and;
	}
	if ($normalized) return(normalize_ipv6("$and/$m1"));
	return(minimize_ipv6("$and/$m1"));
}


function ipv6_ipv4notation($ipv6, $minimize=1) {
	$ipt=normalize_ipv6($ipv6);
	list($ipt, $mask)=explode('/', $ipt);
	$ipt=explode(':', $ipt);
	if (strcasecmp($ipt[5], 'ffff')) return($ipv6); // not in convertable notation
	$ipv4=hex_to_ip(hexdec($ipt[6].$ipt[7]));
	if ($mask) $m="/$mask"; else $m='';
	if ($minimize) {
		$reduce=0;
		$out='';
		for ($i=5; $i>=0; $i--) {
			if ($reduce>0 && $ipt[$i]!='0000') $reduce=-1;
			if ($reduce>0 && $ipt[$i]=='0000') continue;
			if (!$reduce && $ipt[$i]=='0000') {
				$out=':'.$out;
				$reduce=1;
				continue;
			}
			$v=dechex(hexdec($ipt[$i]));
			$out=$v.':'.$out;
		}
		return("$out$ipv4$m");
	} else return("$ipt[0]:$ipt[1]:$ipt[2]:$ipt[3]:$ipt[4]:$ipt[5]:$ipv4$m");
}

function calc_netaddr6($ip, $mask=NULL, $normalized=0) {
	$ip=normalize_ipv6($ip);
	list($i, $m)=explode('/', $ip);
	if ($mask) {
		if (strpos($mask, ':'!==FALSE)) $m=bits_from_mask6($mask);
		else $m=$mask+0;
		if ($m<0) $m=0;
		if ($m>128) $m=128;
	} 
	$mb=netmask_from_bits6($m);
	return(and_addr6("$i/$m", $mb, $normalized));
}

function next_subnet_in_pool($last, $mask) {
	list($a1, $m1)=explode('/', normalize_ipv6($addr1));
	list($a2, $m2)=explode('/', normalize_ipv6($addr2));
	$a1=explode(':', $a1);
	$a2=explode(':', $a2);
	$i=8;
	$sum='';
	$carry=0;
	while($i--){
		$s=hexdec($a1[$i])+hexdec($a2[$i])+$carry;
		if ($s>0xffff) $carry=1; else $carry=0;
		$sum=dechex($s & 0xffff).$sum;
		if ($i) $sum=':'.$sum;
	}
	return(minimize_ipv6("$sum/$m1"));
}


function netaddr_from_bits6($bits) {
	$res=array(0,0,0,0, 0,0,0,0, 0,0,0,0 ,0,0,0,0);
//	if ($bits>0 && $bits<=128) {
		$i=floor($bits/8);
		$r=1 << (8 -($bits % 8));
//echo "ssss".($bits % 8)."dddd\n";
//echo "$bits, $i: $r=1 << (7 -($bits % 8));\n";
		if ($r>255) {
			if ($i) $res[$i-1]=1;
		} else $res[$i]=$r;

		
print_r($res);
//	}
	return(pack('C*', $res[0],$res[1],$res[2],$res[3],$res[4],$res[5],$res[6],$res[7],$res[8],$res[9],
		$res[10],$res[11],$res[12],$res[13],$res[14],$res[15]));
}

function net_from_bits6($bits) {
	if ($bits=='' || $bits>128 || $bits<0) return('0::');
	$m=dechex(1 << ((128-$bits) % 16));
	$d='::';
	for ($last=$bits; $last<=112; $last+=16) {
		$m.=':0';
	}
	if ($bits>16) $m="::$m";
	return($m);
}

function compute_ip_posibility_sql($ip, $max_mask=32, $min_mask=8) {
//echo "compute_ip_posibility_sql($ip, $max_mask=32, $min_mask=16)";
        if (!$ip) return ("''");
        $hip=ip_to_hex($ip);
        $ret='';
        for($i=$max_mask; $i>=$min_mask; $i--) {
            $try_ip=$hip & (0xffffffff ^ (pow(2, 32-$i)-1));
//echo  "$i $try_ip $hip ".(0xffffffff ^ (pow(2, 32-$i)-1))."\n";
            if ($ret) $ret.=', ';
            $ret.="'".hex_to_ip($try_ip);
            $ret.="/$i'";
                        if ($i==32) {
                                if ($ret) $ret.=', '; //nakonec bez masky
                                $ret.="'".hex_to_ip($try_ip)."'";
                        }
        }
        if (!$ret) $ret="''";
//echo "compute_ip_posibility_ret($ret)";
        return($ret);
}



?>