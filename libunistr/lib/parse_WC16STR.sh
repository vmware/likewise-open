#!/bin/bash

INPUT_FILE=$1
if [ -z "$INPUT_FILE" ]; then
    echo "Error: must pass in input file name as first argument"
    exit 1
fi
OUTPUT_FILE=$2
if [ -z "$OUTPUT_FILE" ]; then
    OUTPUT_FILE=/dev/stdout;
fi

SED_ESCAPED_INPUT_FILE=`echo "$INPUT_FILE" | sed -e 's_[/\]_\\\\&_g'`

# Insert the line number on a new line above each original line
# "abc" turns into "1\nabc"
sed -e '=' "$INPUT_FILE" \
`# Pipe the output through a new invocation of sed. In this invocation, do not print out every line by default.` \
    | sed -n -e '
# Tell sed to read two lines at a time, so that "1\nabc" is in the pattern buffer at the same time, instead of "1" then "abc".
    N;

# Apply the following commands only to lines that contain "WC16STR"
    /WC16STR *( *"\(\([^\"]\|\\.\)*\)" *)/{

# Store the pattern buffer in the hold buffer
	h

# Change line number into a variable declaration:
# "50\nstart WC16STR("text") end" -> "const static WCHAR gwsz50[] = {"
    s/^\(.*\)\n.*$/const static WCHAR gwsz\1[] = {/

# Exchange the hold buffer and pattern buffer (restores "50\nstart WC16STR("text") end")
    x

# Select only the string literal from the pattern buffer
# "50\nstart WC16STR("text") end" -> "text"
    s/^.*WC16STR *( *"\(\([^\"]\|\\.\)*\)" *).*$/\1/

# Turn each character of the string into a single quoted character
# text -> 't','e','s','t'
    s/[^\]\|\\./'\''&'\','/g

# Transform single quotes into escaped single quotes
# """ -> "\"" (but with single quotes)
    s/'\'\'\'',/'\'\\\\\'\'',/g

# Append the terminating null
# "t","e","s","t" -> "t","e","s","t",0
    s/^.*$/&0/

# Split long lines at the first comma after the 69th character. This ensures that no line is longer than 75 characters.
    s/.....................................................................,/&\n/g

# Remove the last newline character incase the last line ended with a comma in the 70th position.
    s/\n$//

# Prepend each line with 4 spaces for indentation
    s/^\|\n/&    /g

# Append the pattern buffer to the hold buffer. It will now look like:
# const static WCHAR gwsz50[] = {
#     't','e','s','t',0
    H

# Create a termination for the variable declaration
    s/^.*$/};/

# Append the termination to the hold buffer. It will now look like:
# const static WCHAR gwsz50[] = {
#     't','e','s','t',0
# };
    H

# Copy the hold buffer to the pattern buffer
    g
# Print the pattern buffer
    p
    }' \
`# Pipe the output through a new invocation of sed. In this invocation, insert the string constants on the "WC16STR_INSERT" line. Also start building up the #line directive.` \
    | sed -e '/^[ ]*WC16STR_INSERT[ ]*$/{
# Load the text from the previous sed invocation
    r /dev/stdin
# Read in the next line of the file. This is necessary to tell sed to execute the 'r' command immediately (vs. after the '=' command).
    N
# Remove the insert comment, and add the start of a line directive
    s/^.*\n/#line MERGEWITHLINEABOVE "'$SED_ESCAPED_INPUT_FILE'"\n/
# Print out the line number. The line number ends up above the #line directive
    =
    }' "$INPUT_FILE" \
`# Fix the #line directive` \
    | sed -e '
# Read in the next line of input (so that two lines are in the pattern buffer
    N
# If the second line is the special #line directive, merge the lines together and insert the line number in the correct spot.
    s/^\(.*\)\n#line MERGEWITHLINEABOVE /#line \1 /
# Print out the pattern buffer up to the first newline
    P
# Clear the pattern buffer up to the first newline, and restart flow control at the top of the script (the 'N' command).
    D' >"$OUTPUT_FILE"
