#!/bin/bash

if [ $# -lt  1 ] || [ $# -gt 2 ]; then
  echo "This tool will send either one, or all data files to the web interface"
  echo ""
  echo "Syntax: $0 <hostname or IP address> [<path to a file>]"
  exit 255
fi

IP="$1"
echo "Uploading to $IP"

if [ $# -gt 1 ]; then
  files="$2"
else
  files="./data/*"
fi

for file in $files; do
  echo "Sending: $file"
  # curl -v --trace-ascii - -F 'data=@"'"$file"'";filename="'"$(basename "$file")"'"' http://"$IP"/edit
  curl -F 'data=@"'"$file"'";filename="'"$(basename "$file")"'"' http://"$IP"/edit
done
