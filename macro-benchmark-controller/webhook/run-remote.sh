#!/bin/bash

# Launch remote webhook server
usage="$(basename "$0") [-h] [<optional-hostname>]
Options:
  -h: show help message
  optional-hostname: default value is [syn-bd]
"

while getopts ':h' option; do
  case "$option" in
  h)
    echo "$usage"
    exit
    ;;
  \?)
    printf "illegal option: -%s\n" "$OPTARG" >&2
    echo "$usage" >&2
    exit 1
    ;;
  esac
done

HOST=syn-bd

if [ "$1" != "" ]; then
  HOST="$1"
fi

scp nginx-config.conf $HOST:~/webapp/
scp webhook.ini $HOST:~/webapp/
scp webhookCallback.py $HOST:~/webapp/
ssh $HOST 'sudo service nginx restart'
