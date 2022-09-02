cgdb -p $(ps -aux | grep ./build/mfwm | grep -v grep | awk '{ print $2 }')
