#!/bin/sh
cd $1
# run gdb in batch mode to generate mail the stack trace of the latest crash to the error_recipeint from config.json
/usr/bin/gdb -batch -ex "set pagination off" -ex "bt full" ./bot `ls -Art *core* | tail -n 1` | grep -v ^"No stack."$ | mail -s "Sporks Bot rebooted, stack trace attached" `/usr/bin/jq -r '.error_recipient' ../config.json`

