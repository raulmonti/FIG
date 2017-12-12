#
# Author:  Carlos E. Budde
# Date:    12.12.2017
# License: GPLv3
#
#   Prepare *the* proper propery in the given model file,
#   erasing any other property in the properties...endproperties environment.
#   NOTE: this script should be sourced from another script  XXX
#

# Reliability: "P ( * U * )"
BLANK="[[:space:]]*";
PROPERTY_REGEX="${BLANK}P${BLANK}(.*U.*).*";

# Check invocation
if [[ ! "$(type -t showe 2>&1)" =~ "function" ]]; then
	echo -e "[ERROR] Function \"showe()\" is not in the environment" >&2;
	echo -e "        This script must be *sourced* from main.sh" >&2;
	return 1;
elif [ $# -ne 1 ]; then
	showe "[ERROR] Must pass model file as single parameter";
	return 1;
elif [ ! -f $1 ]; then
	showe "[ERROR] Model file \"$1\" not found";
	return 1;
fi

# Extract property from the given model file
PROPERTIES_SECTION=`mktemp`;
gawk -v _BLK_=$BLANK '
	{if ($0 ~ "^"_BLK_"properties$"_BLK_) {f=1;}}
	f;
	{if ($0 ~ "^"_BLK_"endproperties$"_BLK_) {f=0;}}
	' $1 > $PROPERTIES_SECTION;
NUM_PROPERTIES=$(bc <<< "`cat $PROPERTIES_SECTION | wc -l `-2");
if [ $NUM_PROPERTIES -lt 0 ]; then
	showe "[ERROR] Model file \"$1\" has no properties...endproperties section";
	return 1;
elif [ $NUM_PROPERTIES -eq 0 ]; then
	showe "[ERROR] Model file \"$1\" contains no properties";
	return 1;
fi
PROPERTY=$(gawk -v PROP="$PROPERTY_REGEX" '
	{ if ($0 ~ PROP) {print} }' $PROPERTIES_SECTION);
if [ -z "$PROPERTY" ] || [[ ! $PROPERTY =~ $PROPERTY_REGEX ]]; then
	showe "[ERROR] No valid property found in the model file \"$1\"";
	showe "        (the property regex is: $PROPERTY_REGEX)";
fi

# Remove original properties...endproperties section from the model file
# and replace it with the single property extracted
echo > $PROPERTIES_SECTION;
gawk -v _BLK_=$BLANK '
	{if ($0 ~ "^"_BLK_"properties$"_BLK_) {f=1;}}
	!f;
	{if ($0 ~ "^"_BLK_"endproperties$"_BLK_) {f=0;}}
	' $1 >> $PROPERTIES_SECTION;
######XXX XXX XXX######
# Last line of file must contain the GIF (global ifun) !!!
HORRIBLE_HACK=`mktemp`;
tail -n 2 $PROPERTIES_SECTION > $HORRIBLE_HACK;
#######################
for STR in "" "properties" "$PROPERTY" "endproperties" ""; do
	echo $STR >> $PROPERTIES_SECTION;
done
######XXX XXX XXX######
cat $HORRIBLE_HACK >> $PROPERTIES_SECTION;
rm $HORRIBLE_HACK;
#######################
rm -f $1;
mv $PROPERTIES_SECTION $1;

return 0;
