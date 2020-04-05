#!/bin/bash

set -e;

# Check dependencies
check() {
	command -v "$@" &>/dev/null ||
	( echo "Cannot build documentation: missing dependency \"$@\"" &&
	  exit 1 );
}
check doxygen;
check grep;
check gawk;
if [ ! -f Doxyfile ]; then
	echo "Couldn't find \"Doxyfile\" file, aborting";
	exit 1;
fi

# Build documentation
doxygen Doxyfile;

# Show it
DOC_HTML_DIR=$(grep "^HTML_OUTPUT" Doxyfile | gawk '{print $3}');
DOC_ENTRY_POINT="html-doc.html";
ln -sf ${DOC_HTML_DIR}/index.html ${DOC_ENTRY_POINT};
ln -sf ./doc/${DOC_HTML_DIR}/index.html ../doc.html;
echo;
echo "Opening browser with generated documentation...";
echo;
nohup xdg-open ${DOC_ENTRY_POINT} &> /dev/null &
disown %%;

exit 0;
