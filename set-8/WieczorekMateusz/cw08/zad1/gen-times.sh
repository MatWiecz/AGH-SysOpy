#!/bin/bash
FILTER_LIST=`ls filters`
THREAD_NUM_LIST="1 2 4 8"
PROCESS_METHOD_LIST="block interleaved"

for PROCESS_METHOD in $PROCESS_METHOD_LIST
do
	for FILTER in $FILTER_LIST
	do
		for THREAD_NUM in $THREAD_NUM_LIST
		do
			echo "TEST: Process method: "$PROCESS_METHOD"; Threads number: "$THREAD_NUM"; Filter: "$FILTER >> Times.txt
			echo "Command:" ./image-filterer $THREAD_NUM $PROCESS_METHOD images/mountain.ascii.pgm filters/$FILTER images/processed-$THREAD_NUM-$PROCESS_METHOD-$FILTER.ascii.pgm >> Times.txt
			./image-filterer $THREAD_NUM $PROCESS_METHOD images/mountain.ascii.pgm filters/$FILTER images/processed-$THREAD_NUM-$PROCESS_METHOD-$FILTER.ascii.pgm >> Times.txt
			echo "=========================================================================" >> Times.txt
			echo "" >> Times.txt
		done
	done
done
