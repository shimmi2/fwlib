<?PHP

$boundary="----=_hraprzakladni454cvcbtya5345621";
$boundary_alt="----=_hraalpr56r76dfw0jk90h2";

// Predpokladany tvar mailu :
// root
//  +-- alternative
//  |       +--text/plain telo mailu
//  |       \--text/html telo mailu
//  +--priloha1
//  +  ....
//  \--prilohan
//
// s nicim slozitejsim vetsim knihovna nepocita !!!
// samozrejme, ze je treba volat sekvencne spravne, pro priklad ze zhora 
// init_multipart_msg(), begin_multipart_alternative(), 
// add_multipart_msg_8bit(txt), add_multipart_msg_8bit(html)
// close_multipart_alternative(), add_multipart_msg_base64(priloha1-n)

//function mime_header_encode($text, $encoding = "utf-8") {
//    return "=?$encoding?Q?" . imap_8bit($text) . "?=";
//}
function mime_header_encode($string, $encoding='UTF-8') {
    $string = str_replace(" ", "_", trim($string)) ;
    // We need to delete "=\r\n" produced by imap_8bit() and replace '?'
    $string = str_replace("?", "=3F", str_replace("=\r\n", "", imap_8bit($string))) ;
   
    // Now we split by \r\n - i'm not sure about how many chars (header name counts or not?)
    $string = chunk_split($string, 73);
    // We also have to remove last unneeded \r\n :
    $string = substr($string, 0, strlen($string)-2);
    // replace newlines with encoding text "=?UTF ..."
    $string = str_replace("\r\n", "?=\r\n =?".$encoding."?Q?", $string) ;
   
    return '=?'.$encoding.'?Q?'.$string.'?=';
}

// Udela hlavicky mailu standardne pro "mixed obsah"
function make_multipart_msg_headers($from, $type="mixed") {
  global $boundary;
  $h="From: $from\n";
  $h.="MIME-Version: 1.0\n";
  $h.="Content-Type: multipart/$type; boundary=\"$boundary\"";
  return ($h);
}

function init_multipart_msg() {
  global $boundary;
  $m="This is a multi-part message in MIME format.\n";
  $m.="--$boundary";
  return($m);
}

function end_multipart_msg($m) {
  return($m.'--');
}

//nehotovo
function begin_multipart_alternative() {
}

//nehotovo
function close_multipart_alternative() {
}

function add_multipart_msg_base64($m, $name, $mime, $content) {
  global $boundary;
  $m.= "\nContent-Type: $mime;\n name=\"$name\"\n";
  $m.= "Content-Transfer-Encoding: base64\n";
  $m.= "Content-Disposition: attachment;\n filename=\"$name\"\n\n";
  $m.= chunk_split(base64_encode($content));
  $m.= "--$boundary";
  return ($m);
}

function add_multipart_msg_8bit($m, $mime, $content) {
  global $boundary;
  $m.= "\nContent-Type: $mime;\n";
  $m.= "Content-Transfer-Encoding: 8bit\n\n";
  $m.= $content."\n";
  $m.= "--$boundary";
  return ($m);
}


?>