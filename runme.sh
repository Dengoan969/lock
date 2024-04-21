rm -f *.txt
rm -f *.lck
rm -f lock.o
rm -f lock

make

for i in {1..10}
do
    ./lock myfile.txt &
done

sleep 300

killall -SIGINT lock

cat result.txt