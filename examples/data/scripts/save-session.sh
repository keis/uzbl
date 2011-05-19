#!/bin/sh

if [ -z "$UZBL_UTIL_DIR" ]; then
    # we're being run standalone, we have to figure out where $UZBL_UTIL_DIR is
    # using the same logic as uzbl-browser does.
    UZBL_UTIL_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/uzbl/scripts/util"
    if ! [ -d "$UZBL_UTIL_DIR" ]; then
        PREFIX="$( grep '^PREFIX' "$( which uzbl-browser )" | sed -e 's/.*=//' )"
        UZBL_UTIL_DIR="$PREFIX/share/uzbl/examples/data/scripts/util"
    fi
fi

. "$UZBL_UTIL_DIR/uzbl-dir.sh"

UZBL_SESSION_DIR="$UZBL_DATA_DIR/session"

if [ -n "$1" ]; then
	session="$1"
else
	session="saved"
fi

if [ ! -e "$UZBL_SESSION_DIR/$session" ]; then
	mkdir -p "$UZBL_SESSION_DIR/$session"
fi

cp $UZBL_SESSION_DIR/cur/* "$UZBL_SESSION_DIR/$session"
