encrypt-file() {
        gpg -c --no-symkey-cache $1
        rm $1
}

decrypt-file() {
        gpg -o decrypted --yes -d --no-symkey-cache $1
}

refresh() {
  bash /home/tiny/.config/scripts/proxy_refresh.sh
}

webcam() {
        if [[ $* == '-h' ]]; then
                echo 'Usage:'
                echo 'webcam [options]'
                echo 
                echo 'Open webcam. Can be used to record.'
                echo
                echo 'Options:'
                echo '-r  record with audio'

        elif [[ $* == '-r' ]]; then
                ffmpeg -f v4l2 -video_size 640x480 -i /dev/video0 -f alsa -i default -c:v libx264 -preset ultrafast -c:a aac webcam.mp4

        else
                mpv av://v4l2:/dev/video0 --profile=low-latency --untimed
        fi
}

capture() {
        if [[ $* == '-s' ]]; then
                ffmpeg -f x11grab -video_size 1920x1080 -i $DISPLAY -vframes 1 screen.png 1>/dev/null 2>&1

        elif [[ $* == '-r' ]]; then
                ffmpeg -f x11grab -video_size 1920x1080 -framerate 25 -i $DISPLAY -c:v ffvhuff screen.mkv

        elif [[ $* == '-a' ]]; then
                ffmpeg -f x11grab -video_size 1920x1080 -framerate 25 -i $DISPLAY -f alsa -i default -c:v libx264 -preset ultrafast -c:a aac screen.mp4

        else
                echo 'Usage:'
                echo 'screen-capture <option>'
                echo
                echo 'Captures screen'
                echo
                echo 'Options:'
                echo '-a  record with audio'
                echo '-r  record without audio'
                echo '-s  screenshot'
        fi
}
