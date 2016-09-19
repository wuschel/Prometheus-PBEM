# -*- Ksh -*- // Eigentlich SED, aber emacs hat keinen SED-Modus

# Ueberschriften

s/<heading1>/<table border="0" cellspacing="0" cellpadding="4" width="100%"><tr bgcolor="#d0d0d0"><td><font size="+3"><b>/g
s/<\/heading1>/<\/b><\/font><\/td><\/tr><\/table>/g
s/<caption[[:space:]]\+"\([^"]*\)"[[:space:]]*>/<table border="0" cellspacing="0" cellpadding="4" width="100%"><tr bgcolor="#d0d0d0"><td><font size="+3"><b>\1<\/b><\/font><\/td><\/tr><\/table>/g

s/<heading2>/<p><table><tr><td><img src="ueber2.png"><\/td><td><font size="+2">/g
s/<heading2[[:space:]]\+tag="\([^"]*\)">/<a name="\1"><p><table><tr><td><img src="ueber2.png"><\/td><td><font size="+2">/g
s/<\/heading2>/<\/font><\/td><\/tr><\/table>/g
s/<section[[:space:]]\+"\([^"]*\)"[[:space:]]*>/<p><table><tr><td><img src="ueber2.png"><\/td><td><font size="+2">\1<\/font><\/td><\/tr><\/table>/g

# Navigationsleiste

s/<navigation>/<navigation top>/g
s/<\/navigation>/<\/navigation top>/g
s/<navigation top>//g
s/<\/navigation top>/<hr>/g
s/<navigation bottom>/<hr>/g
s/<\/navigation bottom>//g
s/<up "\([^"]*"\)>/<a href="\1"><img border=0 src="hoch\.png"><\/a>/g
s/<up>/<img src="nhoch\.png">/g

# Itemize Umgebung

s/<itemize>/<p><table cellspacing="5">/g

s/<item>/<tr><td valign=top align=center><img src="knopf.png"><\/td><td width="100%">/g
s/<item image="\([^"]*\)">/<tr><td valign=top align=center><img src="\1"><\/td><td width="100%">/g
s/<\/item>/<\/td><\/tr>/g

s/<example>/<tr><td valign=top align=center><img src="beispiel.png"><\/td><td bgcolor="#d0d0d0" align=center valign=top><table bgcolor="#00c1ff" width="100%" cellpadding="4"><tr><td>/g
s/<\/example>/<\/td><\/tr><\/table><\/td><\/tr>/g

s/<mind>/<tr><td valign=top align=center><img src="merke.png"><\/td><td width="100%">/g
s/<\/mind>/<\/td><\/tr>/g

s/<important>/<tr><td valign=top align=center><img src="wichtig.png"><\/td><td width="100%">/g
s/<\/important>/<\/td><\/tr>/g

s/<hint>/<tr><td valign=top align=center><img src="tip.png"><\/td><td width="100%">/g
s/<\/hint>/<\/td><\/tr>/g

s/<warning>/<tr><td valign=top align=center><img src="achtung.png"><\/td><td width="100%"><b>/g
s/<\/warning>/<\/b><\/td><\/tr>/g

s/<\/itemize>/<\/table>/g

# Ueberschriften der Befehlsbeschreibungen

s/<commandname type="\([a-zA-Z]*\)">\(.*\)<\/commandname>/<!"\2"!><table cellpadding="2" width="100%"><tr><td><font size="+2"><b>\2<\/b><\/font><\/td><td align=right><font size="+2"><b>(\1)<\/b><\/font><\/td><\/tr><\/table>/g
s/<commandname important type="\([a-zA-Z]*\)">\(.*\)<\/commandname>/<!"\2"!><table cellpadding="2" width="100%"><tr><td valign=top><img src="wichtig.png"><\/td><td width="100%"><font size="+2"><b>\2<\/b><\/font><\/td><td align=right><font size="+2"><b>(\1)<\/b><\/font><\/td><\/tr><\/table>/g

s/<command anchor="\([^"]*\)">/<!befehl><a name="\1">/g

s/<commandtitles>/<table border="0" width="100%" bgcolor="#d0d0d0" cellspacing="0">/g
s/<\/commandtitles>/<\/table>/g

s/<commandtitle\( duration="\([^"]*\)"\)*>\(.*\)<\/commandtitle>/<tr><td><font size="\+2">\3<\/font><\/td><td align=right><font size="+2">\&nbsp;\2<\/font><\/td><\tr>/g

# Illustrationen

s/<illustration *image="\([^"]*\)">/<table cellspacing="5"><tr><td valign=top><img src="\1"><\/td><td width="100%" valign=top>/g
s/<\/illustration>/<\/td><\/tr><\/table>/g

# Tabellen

s/<tabular[[:space:]][^>]*>/<table cellspacing="2" border="0" cellpadding="2" bgcolor="#d0d0d0">/g
s/<\/tabular>/<\/table>/g
s/<headrow>/<tr bgcolor="#e0e0e0"><td valign=top><b>/g
s/<\/headrow>/<\/b><\/td><\/tr>/g
s/<headtab>/<\/b><\/td><td valign=top><b>/g
s/<row>/<tr><td valign=top>/g
s/<\/row>/<\/td><\/tr>/g
s/<tab>/<\/td><td valign=top>/g

# Verbatim: Woertlicher Computertext in Schrift mit festem Abstand
s/<verbatim>/<pre><b>/g
s/<\/verbatim>/<\/b><\/pre>/g

# Bilder: <image "filename">
s/<image[[:space:]]\+"\([^"]*\)">/<IMG SRC="\1">/g
