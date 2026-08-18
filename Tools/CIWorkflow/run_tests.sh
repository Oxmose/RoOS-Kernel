#!/bin/bash

mkdir generated
rm -rf generated/*
cd Source

python3 ../Tools/CIWorkflow/TestValidator.py x86_64 Kernel/tests/testing/test_groups_x64.json Kernel/tests/includes/TestList.h ../generated/test_file_output.txt

if (( $? != 0 ))
then
    exit -1
fi
