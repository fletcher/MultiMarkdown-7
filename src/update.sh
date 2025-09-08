#!/bin/sh

re2c -i mmd_line_scanner.re > mmd_line_scanner.c

re2c -i mmd_token_scanner.re > mmd_token_scanner.c
