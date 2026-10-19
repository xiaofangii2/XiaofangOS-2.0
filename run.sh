#!/data/data/com.termux/files/usr/bin/bash
cd "$(dirname "$0")"
[ -f ./mykernel ] || ./build.sh
exec ./mykernel
