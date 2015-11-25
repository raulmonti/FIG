#!/bin/bash

set -e

# Check dependencies
which doxygen &> /dev/null || \
	(echo "Please install doxygen and try again" && exit 1)
which grep &> /dev/null || \
	(echo "Please install grep and try again" && exit 1)
which gawk &> /dev/null || \
	(echo "Please install gawk and try again" && exit 1)
if [ ! -f Doxyfile ]
then
	echo "Couldn't find \"Doxyfile\" file, aborting"
	exit 1
fi

# Build documentation
MAIN_PAGE="mainpage.dox"
rm -f ${MAIN_PAGE}
echo -n "/*! \mainpage " >> ${MAIN_PAGE}
cat ../README.md         >> ${MAIN_PAGE}
echo -e "\n*/"           >> ${MAIN_PAGE}
doxygen Doxyfile

# Show it
DOC_HTML_DIR=$(grep "^HTML_OUTPUT" Doxyfile | gawk '{print $3}')
DOC_ENTRY_POINT="html-doc.html"
rm -f ${DOC_ENTRY_POINT}
ln -s ${DOC_HTML_DIR}/index.html ${DOC_ENTRY_POINT}

echo "Opening browser with generated documentation..."
xdg-open ${DOC_ENTRY_POINT} &> /dev/null &

exit 0
