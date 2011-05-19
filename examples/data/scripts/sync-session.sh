#!/bin/sh

. "$UZBL_UTIL_DIR/uzbl-dir.sh"

UZBL_SESSION_DIR="$UZBL_DATA_DIR/session"

if [ ! -e "$UZBL_SESSION_DIR/cur" ]; then
	mkdir -p "$UZBL_SESSION_DIR/cur"
fi

if [ "$1" = 'exit' ]; then
	unlink "$UZBL_SESSION_DIR/cur/$UZBL_XID"
else
	echo "$UZBL_URI" >> "$UZBL_SESSION_DIR/cur/$UZBL_XID"
fi
