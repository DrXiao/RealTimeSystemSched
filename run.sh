for i in $(seq 1 7)
do
    ./main test/test$i.txt > output/output$i.txt 2> output/err$i.txt
done