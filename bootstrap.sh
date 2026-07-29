#!/usr/bin/env bash

apt-get update

apt-get upgrade cmake

# Install development packages for MMD
apt-get install -y build-essential cmake libbsd-dev

# GTK-Editor

# Development stuff
apt-get install -y valgrind

# llvm/fuzzing tools
apt-get install -y clang llvm

# Cross-compilation for Windows
apt-get install -y mingw-w64 nsis

# libcurl development
apt-get install -y libcurl4-openssl-dev


# Create a git server

# Create account
if [ ! -d /home/git ]; then
	useradd git --create-home --shell /bin/bash 
	
	mkdir /home/git/.ssh

	# Use authorized keys file for public key authentication

	# You will need to have generated this file and put it in 
	# the directory prior to provisioning
	cp /vagrant/authorized_keys /home/git/.ssh/authorized_keys

	# Fix permissions
	chown -R git:git /home/git/.ssh
	chmod go-w /home/git
	chmod 700 /home/git/.ssh
	chmod 600 /home/git/.ssh/authorized_keys
fi

