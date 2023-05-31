#!/bin/sh
HOST=$(hostname)
case "$HOST" in
  *rabpm-*) echo $(echo "$HOST" | tr -d -c 0-9) ;;
  *rabpmtl*) echo 21
esac
