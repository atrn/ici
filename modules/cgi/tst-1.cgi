#!/usr/local/bin/ici

cgi.start_reply_header();
cgi.content_type("text/plain");
cgi.end_reply_header();

printf("This is working!\n");
