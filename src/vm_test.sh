#/bin/bash

echo "Evaluating Test Cases on VM";
cd /home/aqualab/.aqualab/project1;
make test-reg SHELL_ARCH=32;
