#!/bin/bash

PROG="md5sum-asan"
PROGOPT="-c"
INPUT_PATTERN="inputs/bin-ls-md5s-fuzzed-%s"
INPUT_CLEAN="inputs/bin-ls-md5s"

output="result"

echo "Checking if buggy ${PROG} succeeds on non-trigger input..."
./clang-install/${PROG} ${PROGOPT} ${INPUT_CLEAN} &> /dev/null 
rv=$?
if [ $rv -lt 128 ]; then
    echo "Success: ${PROG} ${PROGOPT} ${INPUT_CLEAN} returned $rv"
else
    echo "ERROR: ${PROG} ${PROGOPT} ${INPUT_CLEAN} returned $rv"
fi

echo "Validating bugs..."
mkdir -p $output
cat validated_bugs | while read line ; do
    INPUT_FUZZ=$(printf "$INPUT_PATTERN" $line)
    # { ./coreutils-8.24-lava-safe/lava-install/bin/${PROG} ${PROGOPT} ${INPUT_FUZZ} ; } &> /dev/null
    { ./clang-install/${PROG} ${PROGOPT} ${INPUT_FUZZ} ; } &> $output/$line
    echo $line $?
done > validated.txt

# the original approach is checking the exit code, if the value is greater than 128, it is treated as a validated bug
# however, sometimes the software exits with other value.
# Note the software will print `Successfully triggered bug XXX, crashing now!`, we use it to identify whether a bug is validated

# awk 'BEGIN {valid = 0} $2 > 128 { valid += 1 } END { print "Validated", valid, "/", NR, "bugs" }' validated.txt

# echo "You can see validated.txt for the exit code of each buggy version."

# use a python script to check the correctness
eval "python3 ./validate_res.py $output"
