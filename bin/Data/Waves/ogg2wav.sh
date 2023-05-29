for f in *.ogg; do ffmpeg -i "$f" "${f/%ogg/wav}"; done



