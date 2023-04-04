# Author: Yohan Hmaiti, Anna Zheng, Alyssa Yeekee
    echo "Welcome to the PA2 COP 4600 - Input Device made by Yohan Hmaiti, Anna Zheng, Alyssa Yeekee!"
    echo "Please enter a message when prompted, it will be send to the output device for printing...."
    while true; do
        read -p "Enter some input -> " input
        echo "$input" > /dev/pa2OS_in
    done