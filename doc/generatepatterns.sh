#!/bin/sh

for i in {1..1000000}; do
	length=$((0x`openssl rand 1 -hex`))
	echo	`openssl rand -hex $length`.{`uuidgen`}
done;
