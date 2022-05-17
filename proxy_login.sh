#!/bin/bash

export http_proxy=http://gateway.iitmandi.ac.in:8080/
export ftp_proxy=ftp://gateway.iitmandi.ac.in:8080/
export https_proxy=http://gateway.iitmandi.ac.in:8080/


# check if we are behind IIT Mandi's captive portal and not logged in
if [[ -n $(curl -L http://neverssl.com | grep "IIT Mandi|Internet Access") ]]; then

  # kill any previous tmux session
  SESSIONID=$(cat /tmp/tmux_sessionid)
  if [[ -n $SESSIONID ]]; then
    tmux kill-session $SESSONID
    rm /tmp/tmux_sessionid
  fi

  # run the command in detached tmux session
  tmux new -d 'w3m -post - https://stgw.iitmandi.ac.in/ug/authenticate.php <<<"username=USERNAME&password=PASSWORD&"'
  # store session id of started tmux session
  SESSIONID=$(tmux ls | tail -n 1 | grep -oE "^[0-9]*")
  echo -n $SESSIONID > /tmp/tmux_sessionid
  chmod 600 /tmp/tmux_sessionid

  # send success notification with IP
  
  COUNT=0
  SUCCESS=$(tmux capture-pane -p -t $SESSIONID | grep 'success.php')
  while [[ -z $SUCCESS && $COUNT -lt 60 ]]
  do
    sleep 1
    SUCCESS=$(tmux capture-pane -p -t $SESSIONID | grep 'success.php')
    ((COUNT=COUNT+1))
  done

  tmux send-keys -t $SESSIONID jjjjjjjjjjjjjjjjjjjjjjjjjjjjjj
  IPADDR=$(tmux capture-pane -p -t $SESSIONID | grep 'IP Address' | awk '{print $3}');
  notify-send "Detected Captive Portal" "Successfully logged in with IP: $IPADDR"

fi

# w3m POSTs request with last attribute having newline for some reason
# w3m -post - <url> <<< "a=a" -> {"a": "a\n"}
# w3m -post - <url> <<< "a=a&" -> {"a": "a", "\n": ""}
