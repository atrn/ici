#!/usr/local/bin/ici

static
dump_form(fields)
{
    cgi.start_reply_header();
    cgi.content_type("text/plain");
    cgi.end_reply_header();
    k := v := NULL;
    forall (k in sort(keys(fields)))
    {
	printf("%s => ", k);
	v := fields[k];
	switch (typeof(v))
	{
	case "string":
	    printf("%s\n", v);
	    break;
	case "array":
	    printf("\n");
	    for (i := 0; i < nels(v); ++i)
		 printf("\t%d: %s\n", i, v[i]);
	    break;
	}
    }
}

static
make_form()
{
    cgi.start_reply_header();
    cgi.content_type("text/html");
    cgi.end_reply_header();

    printf
    (
	"<html>"
	"<body>"
	"<form action=%s>"
	"<b>a</b><input type=text name=a><br>"
	"<b>b</b><input type=text name=b><br>"
	"<b>c</b><input type=checkbox name=c value=v1>"
	"<b>c</b><input type=checkbox name=c value=v2>"
	"<p>"
	"<input type=submit>"
	"</form>"
	"</body>"
	"</html>",
	getenv("SCRIPT_URI")
    );
}

try
{
    fields := cgi.form_data();
    dump_form(fields);
}
onerror
{
    if (error != "no query string")
	fail(error);
    make_form();
}

