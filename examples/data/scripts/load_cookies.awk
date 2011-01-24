#!/bin/awk -f

BEGIN {
	FS="\t"
	scheme["TRUE"] = "https";
	scheme["FALSE"] = "http";
}
{
	gsub("@", "\\@");
	gsub("\\\\","\\\\");
}
$0 ~ /^#HttpOnly_/ {
	if (NF == 7)
		printf("add_cookie \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"\n",
			substr($1,length("#HttpOnly_"),length($1)), $3, $6, $7, scheme[$4], $5)
}
$0 !~ /^#/ {
	if (NF == 7)
		printf("add_cookie \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"\n",
			$1, $3, $6, $7, scheme[$4], $5)
}

