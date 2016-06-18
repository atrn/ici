CP=cp
HTML=html

all	: $(HTML)/ici.css $(HTML)/ici.html $(HTML)/ici.1.html $(HTML)/ici-index.html

$(HTML)/ici.css: ici.css
	$(CP) ici.css $(HTML)

$(HTML)/ici.html: ici.html fix-fm-html.ici
	ici fix-fm-html.ici ici.html $(HTML)/ici.html
	ici fix-fm-html.ici ici.1.html $(HTML)/ici.1.html
	$(CP) *.gif $(HTML)

$(HTML)/ici-index.html : mk-index.ici ici-index.txt
	ici mk-index.ici ici-index.txt $(HTML)/ici-index.html