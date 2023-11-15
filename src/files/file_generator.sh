#!/bin/bash
FILE_SIZE="$1"
FILE_NAME="$2"
fallocate -l "${FILE_SIZE}G" "${FILE_NAME}"
