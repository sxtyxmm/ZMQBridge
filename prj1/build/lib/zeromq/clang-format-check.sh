#!/bin/sh
FAILED=0
IFS=";"
FILES="../../../include/prj1.h;../../../src/examples/client.cpp;../../../src/examples/server.cpp;../../../src/prj1.cpp;../../../tests/test_edge_cases.cpp;../../../tests/test_multithreading.cpp;../../../tests/test_pub_sub.cpp;../../../tests/test_push_pull.cpp;../../../tests/test_req_rep.cpp"
IDS=$(echo -en "\n\b")
for FILE in $FILES
do
	clang-format -style=file -output-replacements-xml "$FILE" | grep "<replacement " >/dev/null &&
    {
      echo "$FILE is not correctly formatted"
	  FAILED=1
	}
done
if [ "$FAILED" -eq "1" ] ; then exit 1 ; fi
