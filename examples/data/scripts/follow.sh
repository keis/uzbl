#!/bin/sh
# This scripts acts on the return value of followLinks in follow.js

. "$UZBL_UTIL_DIR/uzbl-util.sh"

result="$1"
shift

case "$result" in
    XXXFORM_ACTIVEXXX)
        # a form element was selected
        uzbl_control 'event KEYCMD_CLEAR\n'
        ;;
    XXXRESET_MODEXXX)
        # a link was selected, reset uzbl's input mode
        uzbl_control 'set mode=\nevent KEYCMD_CLEAR\n'
        ;;
    XXXNEW_WINDOWXXX*)
        uri="${result#XXXNEW_WINDOWXXX}"
        safe_uri=$( echo "$uri" | uzbl_escape )

        # a link was selected, reset uzbl's input mode
        uzbl_control 'set mode=\nevent KEYCMD_CLEAR\nevent NEW_WINDOW '"$safe_uri"'\n'
        ;;
    XXXRETURNED_URIXXX*)
        uriaction="$1"
        shift

        uri="${result#XXXRETURNED_URIXXX}"
        safe_uri=$( echo "$uri" | uzbl_escape )

        uzbl_control 'set mode=\nevent KEYCMD_CLEAR\n'

        [ -z "$uri" ] && exit

        case "$uriaction" in
            set)
                uzbl_control 'uri '"$safe_uri"'\n'
                ;;
            clipboard)
                print "$uri" | xclip
                ;;
        esac
        ;;
esac
